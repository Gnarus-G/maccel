mod action;
mod component;
mod components;
pub mod context;
mod event;
mod graph_linear;
mod graph_natural;
mod sens_fns;

use action::Action;
use component::TuiComponent;
use components::{ParameterInput, Screen};
use context::{AccelMode, ContextRef, TuiContext};
use crossterm::{
    event::{KeyCode, KeyEventKind},
    terminal::{EnterAlternateScreen, LeaveAlternateScreen},
};
use event::{Event, EventHandler};
use graph_linear::LinearCurveGraph;
use graph_natural::NaturalCurveGraph;
use ratatui::{prelude::CrosstermBackend, Terminal};
use tracing::debug;

use crate::{
    inputspeed,
    params::{
        get_paramater, linear::ALL_LINEAR_PARAMS, natural::ALL_NATURAL_PARAMS, set_parameter,
        ALL_PARAMS,
    },
};

#[derive(Debug)]
struct App {
    context: ContextRef,
    screens: Vec<Screen>,
    accel_mode_circ_idx: usize,
    is_running: bool,

    last_tick_at: Instant,
}

impl App {
    fn new() -> Self {
        let context = ContextRef::new(TuiContext {
            current_mode: get_paramater(AccelMode::PARAM_NAME)
                .map(|mode_tag| match mode_tag.as_str() {
                    "0" => AccelMode::Linear,
                    "1" => AccelMode::Natural,
                    _ => AccelMode::Linear,
                })
                .expect("Failed to read a kernel parameter to get the acceleration mode desired"),
            parameters: ALL_PARAMS.iter().map(|&p| (p).into()).collect(),
        });

        Self {
            screens: vec![
                Screen::new(
                    AccelMode::Linear,
                    ALL_LINEAR_PARAMS
                        .iter()
                        .filter_map(|&p| context.get().parameter(p).copied())
                        .map(|param| ParameterInput::new(&param, context.clone()))
                        .collect(),
                    Box::new(LinearCurveGraph::new(context.clone())),
                ),
                Screen::new(
                    AccelMode::Natural,
                    ALL_NATURAL_PARAMS
                        .iter()
                        .filter_map(|&p| context.get().parameter(p).copied())
                        .map(|param| ParameterInput::new(&param, context.clone()))
                        .collect(),
                    Box::new(NaturalCurveGraph::new(context.clone())),
                ),
            ],
            accel_mode_circ_idx: context.clone().get().current_mode.ordinal() as usize,
            context,
            is_running: true,
            last_tick_at: Instant::now(),
        }
    }

    fn tick(&mut self) -> bool {
        #[cfg(not(debug_assertions))]
        let tick_rate = 16;

        #[cfg(debug_assertions)]
        let tick_rate = 100;

        let do_tick = self.last_tick_at.elapsed() >= Duration::from_millis(tick_rate);
        do_tick.then(|| {
            self.last_tick_at = Instant::now();
        });
        do_tick
    }

    fn selected_screen_idx(&self) -> usize {
        self.accel_mode_circ_idx % self.screens.len()
    }
}

impl App {
    fn handle_event(&mut self, event: &event::Event, actions: &mut action::Actions) {
        debug!("received event: {:?}", event);
        if let Event::Key(crossterm::event::KeyEvent {
            kind: KeyEventKind::Press,
            code,
            ..
        }) = event
        {
            match code {
                KeyCode::Char('q') => {
                    self.is_running = false;
                    return;
                }
                KeyCode::Right => {
                    if self.screens.len() > 1 {
                        self.accel_mode_circ_idx = self.accel_mode_circ_idx.wrapping_add(1);
                        actions.push(Action::SetMode(
                            self.screens[self.selected_screen_idx()].accel_mode,
                        ));
                    }
                }
                KeyCode::Left => {
                    if self.screens.len() > 1 {
                        self.accel_mode_circ_idx = self.accel_mode_circ_idx.wrapping_sub(1);
                        actions.push(Action::SetMode(
                            self.screens[self.selected_screen_idx()].accel_mode,
                        ));
                    }
                }
                _ => {}
            }
        }

        let screen_idx = self.selected_screen_idx();
        let screen = &mut self.screens[screen_idx];
        screen.handle_event(event, actions);
    }

    fn update(&mut self, actions: &mut Vec<action::Action>) {
        debug!("performing actions: {actions:?}");

        for action in actions.drain(..) {
            if let Action::SetMode(accel_mode) = action {
                self.context.get_mut().current_mode = accel_mode;
                set_parameter(AccelMode::PARAM_NAME, accel_mode.ordinal())
                    .expect("Failed to set kernel param to change modes");
                self.context.get_mut().reset_current_parameters();
            }

            let screen_idx = self.selected_screen_idx();
            let screen = &mut self.screens[screen_idx];
            screen.update(&action);
        }
    }

    fn draw(&self, frame: &mut ratatui::Frame, area: ratatui::prelude::Rect) {
        let screen_idx = self.selected_screen_idx();
        self.screens[screen_idx].draw(frame, area);
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
        if let Some(event) = tui.events.next()? {
            app.handle_event(&event, &mut actions);
            app.update(&mut actions);
        }

        if app.tick() {
            app.update(&mut vec![Action::Tick]);
            tui.draw(&mut app)?;
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
