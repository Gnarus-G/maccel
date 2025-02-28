mod action;
mod component;
mod components;
mod context;
mod event;

use action::Action;
use component::TuiComponent;
use components::Screen;
use context::{Context, ContextRef};
use crossterm::{
    event::{KeyCode, KeyEventKind},
    terminal::{EnterAlternateScreen, LeaveAlternateScreen},
};
use event::{Event, EventHandler};
use ratatui::{prelude::CrosstermBackend, Terminal};
use tracing::debug;

use crate::{inputspeed, params::Param};

#[derive(Debug)]
struct App {
    context: ContextRef,
    screens: Vec<Screen>,
    is_running: bool,

    last_tick_at: Instant,
}

impl App {
    fn new() -> Self {
        let context = ContextRef::new(Context {
            parameters: vec![
                Param::SensMult.into(),
                Param::Accel.into(),
                Param::Offset.into(),
                Param::OutputCap.into(),
            ],
        });

        let screen = Screen::new(context.clone());
        Self {
            context,
            screens: vec![screen],
            is_running: true,
            last_tick_at: Instant::now(),
        }
    }

    fn tick(&mut self) -> bool {
        let do_tick = self.last_tick_at.elapsed() >= Duration::from_millis(16);
        do_tick.then(|| {
            self.last_tick_at = Instant::now();
        });
        do_tick
    }
}

impl App {
    fn handle_event(&mut self, event: &event::Event, actions: &mut action::Actions) {
        debug!("received event: {:?}", event);
        if let Event::Key(key) = event {
            if key.kind == KeyEventKind::Press && key.code == KeyCode::Char('q') {
                self.is_running = false;
                return;
            }
        }

        for screen in self.screens.iter_mut() {
            screen.handle_event(event, actions);
        }
    }

    fn update(&mut self, actions: &mut Vec<action::Action>) {
        debug!("performing actions: {actions:?}");
        for action in actions.drain(..) {
            for screen in self.screens.iter_mut() {
                screen.update(&action);
            }
        }
    }

    fn draw(&mut self, frame: &mut ratatui::Frame, area: ratatui::prelude::Rect) {
        let screen = &mut self.screens[0];
        screen.draw(frame, area);
    }
}

pub fn run_tui() -> anyhow::Result<()> {
    let mut app = App::new();

    let backend = CrosstermBackend::new(io::stdout());
    let terminal = Terminal::new(backend)?;
    let events = EventHandler::new();

    let mut tui = Tui::new(terminal, events);
    tui.init()?;

    inputspeed::setup_input_speed_reader();

    let mut actions = vec![];
    while app.is_running {
        tui.draw(&mut app)?;

        if let Some(event) = tui.events.next()? {
            app.handle_event(&event, &mut actions);
        }

        app.update(&mut actions);

        if app.tick() {
            app.update(&mut vec![Action::Tick]);
        }
    }

    tui.exit()?;
    Ok(())
}

use crossterm::event::{DisableMouseCapture, EnableMouseCapture};
use ratatui::backend::Backend;
use std::{io, time::Instant};
use std::{panic, time::Duration};

/// Representation of a terminal user interface.
///
/// It is responsible for setting up the terminal,
/// initializing the interface and handling the draw events.
#[derive(Debug)]
struct Tui<B: Backend> {
    /// Interface to the Terminal.
    terminal: Terminal<B>,
    /// Terminal event handler.
    pub events: EventHandler,
}

impl<B: Backend> Tui<B> {
    /// Constructs a new instance of [`Tui`].
    pub fn new(terminal: Terminal<B>, events: EventHandler) -> Self {
        Self { terminal, events }
    }

    /// Initializes the terminal interface.
    ///
    /// It enables the raw mode and sets terminal properties.
    pub fn init(&mut self) -> anyhow::Result<()> {
        crossterm::terminal::enable_raw_mode()?;
        crossterm::execute!(io::stdout(), EnterAlternateScreen, EnableMouseCapture)?;

        // Define a custom panic hook to reset the terminal properties.
        // This way, you won't have your terminal messed up if an unexpected error happens.
        let panic_hook = panic::take_hook();
        panic::set_hook(Box::new(move |panic| {
            Self::reset().expect("failed to reset the terminal");
            panic_hook(panic);
        }));

        self.terminal.hide_cursor()?;
        self.terminal.clear()?;
        Ok(())
    }

    /// [`Draw`] the terminal interface by [`rendering`] the widgets.
    ///
    /// [`Draw`]: ratatui::Terminal::draw
    /// [`rendering`]: crate::ui::render
    pub fn draw(&mut self, app: &mut App) -> anyhow::Result<()> {
        self.terminal.draw(|frame| app.draw(frame, frame.size()))?;
        Ok(())
    }

    /// Resets the terminal interface.
    ///
    /// This function is also used for the panic hook to revert
    /// the terminal properties if unexpected errors occur.
    fn reset() -> anyhow::Result<()> {
        crossterm::terminal::disable_raw_mode()?;
        crossterm::execute!(io::stdout(), LeaveAlternateScreen, DisableMouseCapture)?;
        Ok(())
    }

    /// Exits the terminal interface.
    ///
    /// It disables the raw mode and reverts back the terminal properties.
    pub fn exit(&mut self) -> anyhow::Result<()> {
        Self::reset()?;
        self.terminal.show_cursor()?;
        Ok(())
    }
}
