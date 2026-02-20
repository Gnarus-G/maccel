use crossterm::terminal::{EnterAlternateScreen, LeaveAlternateScreen};
use maccel_core::ALL_COMMON_PARAMS;
use maccel_core::ALL_LINEAR_PARAMS;
use maccel_core::ALL_MODES;
use maccel_core::ALL_NATURAL_PARAMS;
use maccel_core::ALL_NOACCEL_PARAMS;
use maccel_core::ALL_SYNCHRONOUS_PARAMS;
use maccel_core::Param;
use maccel_core::get_param_value_from_ctx;
use maccel_core::persist::ParamStore;
use maccel_core::persist::SysFsStore;
use maccel_core::profiles::{Profile, ProfileStore};
use maccel_core::{ALL_PARAMS, AccelMode, ContextRef, TuiContext};
use ratatui::Terminal;
use ratatui::backend::Backend;
use ratatui::crossterm::event::{DisableMouseCapture, EnableMouseCapture, KeyCode, KeyEventKind};
use std::{io, time::Instant};
use std::{panic, time::Duration};
use tracing::debug;

use crate::action::{Action, Actions};
use crate::component::TuiComponent;
use crate::event::{Event, EventHandler};
use crate::graph::SensitivityGraph;
use crate::param_input::ParameterInput;
use crate::screen::Screen;
use crate::utils::CyclingIdx;

pub struct App {
    context: ContextRef<SysFsStore>,
    screens: Vec<Screen<SysFsStore>>,
    screen_idx: CyclingIdx,
    pub(crate) is_running: bool,

    profile_dialog_open: bool,
    profiles: Vec<Profile>,
    selected_profile_idx: usize,

    last_tick_at: Instant,
}

pub fn collect_inputs_for_params(
    params: &[Param],
    context: ContextRef<SysFsStore>,
) -> Vec<ParameterInput<SysFsStore>> {
    ALL_COMMON_PARAMS
        .iter()
        .chain(params)
        .filter_map(|&p| context.get().parameter(p).copied())
        .map(|param| ParameterInput::new(&param, context.clone()))
        .collect()
}

impl App {
    pub fn new() -> Self {
        let context = ContextRef::new(
            TuiContext::init(SysFsStore, ALL_PARAMS).expect("Failed to initialize the Tui Context"),
        );

        Self {
            screens: vec![
                Screen::new(
                    AccelMode::Linear,
                    collect_inputs_for_params(ALL_LINEAR_PARAMS, context.clone()),
                    Box::new(
                        SensitivityGraph::new(context.clone()).on_y_axix_bounds_update(|ctx| {
                            // Appropriate dynamic bounds for the Linear sens graph
                            let upper_bound = f64::from(get_param_value_from_ctx!(ctx, SensMult))
                                * f64::from(get_param_value_from_ctx!(ctx, OutputCap)).max(1.0)
                                * 2.0;

                            [0.0, upper_bound]
                        }),
                    ),
                ),
                Screen::new(
                    AccelMode::Natural,
                    collect_inputs_for_params(ALL_NATURAL_PARAMS, context.clone()),
                    Box::new(
                        SensitivityGraph::new(context.clone()).on_y_axix_bounds_update(|ctx| {
                            // Appropriate dynamic bounds for the Natural sens graph
                            let upper_bound = f64::from(get_param_value_from_ctx!(ctx, SensMult))
                                * f64::from(get_param_value_from_ctx!(ctx, Limit)).max(1.0)
                                * 2.0;

                            [0.0, upper_bound]
                        }),
                    ),
                ),
                Screen::new(
                    AccelMode::Synchronous,
                    collect_inputs_for_params(ALL_SYNCHRONOUS_PARAMS, context.clone()),
                    Box::new(
                        SensitivityGraph::new(context.clone()).on_y_axix_bounds_update(|ctx| {
                            // Appropriate dynamic bounds for the Synchronous sens graph
                            let upper_bound = f64::from(get_param_value_from_ctx!(ctx, SensMult))
                                * f64::from(get_param_value_from_ctx!(ctx, Motivity)).max(1.0)
                                * 2.0;

                            [0.0, upper_bound]
                        }),
                    ),
                ),
                Screen::new(
                    AccelMode::NoAccel,
                    collect_inputs_for_params(ALL_NOACCEL_PARAMS, context.clone()),
                    Box::new(
                        SensitivityGraph::new(context.clone()).on_y_axix_bounds_update(|ctx| {
                            // Appropriate dynamic bounds for the NoAccel sens graph
                            let upper_bound =
                                f64::from(get_param_value_from_ctx!(ctx, SensMult)) * 2.0; // No other factor, sens is 1.0

                            [0.0, upper_bound.max(1.0)] // Ensure at least a small visible range
                        }),
                    ),
                ),
            ],
            screen_idx: CyclingIdx::new_starting_at(
                ALL_MODES.len(),
                context.clone().get().current_mode.ordinal() as usize,
            ),
            context,
            is_running: true,
            profile_dialog_open: false,
            profiles: Vec::new(),
            selected_profile_idx: 0,
            last_tick_at: Instant::now(),
        }
    }

    pub(crate) fn tick(&mut self) -> bool {
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

    fn current_screen_mut(&mut self) -> &mut Screen<SysFsStore> {
        let screen_idx = self.screen_idx.current();
        self.screens.get_mut(screen_idx).unwrap_or_else(|| {
            panic!(
                "Failed to get a Screen for mode id {}: {:?}",
                screen_idx, ALL_MODES[screen_idx]
            );
        })
    }

    fn current_screen(&self) -> &Screen<SysFsStore> {
        let screen_idx = self.screen_idx.current();
        self.screens.get(screen_idx).unwrap_or_else(|| {
            panic!(
                "Failed to get a Screen for mode id {}: {:?}",
                screen_idx, ALL_MODES[screen_idx]
            );
        })
    }

    fn can_switch_screens(&self) -> bool {
        self.screens.len() > 1 && !self.current_screen().is_in_editing_mode()
    }
}

impl App {
    pub(crate) fn handle_event(&mut self, event: &Event, actions: &mut Actions) {
        debug!("received event: {:?}", event);
        if let Event::Key(crossterm::event::KeyEvent {
            kind: KeyEventKind::Press,
            code,
            ..
        }) = event
        {
            if self.profile_dialog_open {
                match code {
                    KeyCode::Char('p') | KeyCode::Esc => {
                        self.profile_dialog_open = false;
                    }
                    KeyCode::Up => {
                        if !self.profiles.is_empty() {
                            self.selected_profile_idx = self.selected_profile_idx.saturating_sub(1);
                        }
                    }
                    KeyCode::Down => {
                        if !self.profiles.is_empty() {
                            self.selected_profile_idx =
                                (self.selected_profile_idx + 1).min(self.profiles.len() - 1);
                        }
                    }
                    KeyCode::Enter => {
                        if !self.profiles.is_empty() {
                            let profile = &self.profiles[self.selected_profile_idx];
                            if let Err(e) = profile.apply() {
                                tracing::error!("Failed to apply profile: {}", e);
                            } else {
                                self.context.get_mut().current_mode = profile.mode;
                                self.context.get_mut().reset_current_parameters();
                            }
                            self.profile_dialog_open = false;
                        }
                    }
                    _ => {}
                }
                return;
            }

            match code {
                KeyCode::Char('q') => {
                    self.is_running = false;
                    return;
                }
                KeyCode::Char('p') => {
                    self.profile_dialog_open = true;
                    self.profiles = ProfileStore::new()
                        .and_then(|s| s.list())
                        .unwrap_or_default();
                    self.selected_profile_idx = 0;
                    return;
                }
                KeyCode::Right => {
                    if self.can_switch_screens() {
                        self.screen_idx.forward();
                        actions.push(Action::SetMode(
                            self.screens[self.screen_idx.current()].accel_mode,
                        ));
                    }
                }
                KeyCode::Left => {
                    if self.can_switch_screens() {
                        self.screen_idx.back();
                        actions.push(Action::SetMode(
                            self.screens[self.screen_idx.current()].accel_mode,
                        ));
                    }
                }
                _ => {}
            }
        }

        self.current_screen_mut().handle_event(event, actions);
    }

    pub(crate) fn update(&mut self, actions: &mut Vec<Action>) {
        debug!("performing actions: {actions:?}");

        for action in actions.drain(..) {
            if let Action::SetMode(accel_mode) = action {
                self.context.get_mut().current_mode = accel_mode;
                SysFsStore
                    .set_current_accel_mode(accel_mode)
                    .expect("Failed to set accel mode in TUI");
                self.context.get_mut().reset_current_parameters();
            }

            self.current_screen_mut().update(&action);
        }
    }

    pub(crate) fn draw(&self, frame: &mut ratatui::Frame, area: ratatui::prelude::Rect) {
        self.current_screen().draw(frame, area);

        if self.profile_dialog_open {
            self.draw_profile_dialog(frame, area);
        }
    }

    fn draw_profile_dialog(&self, frame: &mut ratatui::Frame, area: ratatui::prelude::Rect) {
        use ratatui::widgets::{Block, Borders, List, ListItem};

        let popup_width = 40.min(area.width.saturating_sub(4));
        let popup_height =
            (self.profiles.len().max(3) as u16 + 4).min(area.height.saturating_sub(4));

        let x = (area.width - popup_width) / 2;
        let y = (area.height - popup_height) / 2;

        let popup_area = ratatui::prelude::Rect::new(x, y, x + popup_width, y + popup_height);

        let items: Vec<ListItem> = if self.profiles.is_empty() {
            vec![ListItem::new("No profiles found. Use CLI to create one.")]
        } else {
            self.profiles
                .iter()
                .enumerate()
                .map(|(i, p)| {
                    let suffix = if i == self.selected_profile_idx {
                        " ←"
                    } else {
                        ""
                    };
                    ListItem::new(format!("{} ({}){}", p.name, p.mode.as_title(), suffix))
                })
                .collect()
        };

        let list = List::new(items)
            .block(Block::new().borders(Borders::ALL).title("Select Profile"))
            .highlight_style(
                ratatui::style::Style::default().add_modifier(ratatui::style::Modifier::REVERSED),
            );

        frame.render_widget(list, popup_area);
    }
}

/// Representation of a terminal user interface.
///
/// It is responsible for setting up the terminal,
/// initializing the interface and handling the draw events.
#[derive(Debug)]
pub struct Tui<B: Backend> {
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
        self.terminal.draw(|frame| app.draw(frame, frame.area()))?;
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
