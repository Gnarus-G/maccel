use maccel_core::inputspeed;

use event::EventHandler;
use ratatui::{prelude::CrosstermBackend, Terminal};

mod event;

mod action;
mod app;
mod component;
mod graph;
mod graph_linear;
mod graph_natural;
mod param_input;
mod screen;
mod utils;

pub use utils::get_current_accel_mode;

pub fn run_tui() -> anyhow::Result<()> {
    let mut app = app::App::new();

    let backend = CrosstermBackend::new(std::io::stdout());
    let terminal = Terminal::new(backend)?;
    let events = EventHandler::new();

    let mut tui = app::Tui::new(terminal, events);
    tui.init()?;

    inputspeed::setup_input_speed_reader();

    let mut actions = vec![];
    while app.is_running {
        if let Some(event) = tui.events.next()? {
            app.handle_event(&event, &mut actions);
            app.update(&mut actions);
        }

        if app.tick() {
            app.update(&mut vec![action::Action::Tick]);
            tui.draw(&mut app)?;
        }
    }

    tui.exit()?;
    Ok(())
}
