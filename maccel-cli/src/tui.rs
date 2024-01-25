use crossterm::{
    event::{self, Event, KeyCode, KeyEventKind},
    terminal::{disable_raw_mode, enable_raw_mode, EnterAlternateScreen, LeaveAlternateScreen},
    ExecutableCommand,
};
use ratatui::{
    layout::{Constraint, Direction, Layout},
    prelude::{CrosstermBackend, Stylize, Terminal},
    text::{Line, Text},
    widgets::{Block, Borders, Paragraph},
    Frame,
};
use std::io::stdout;
use tui_input::{backend::crossterm::EventHandler, Input};

enum InputMode {
    Normal,
    Editing,
}

struct ParameterInput {
    name: &'static str,
    value: String,
    input: Input,
    input_mode: InputMode,
}

impl ParameterInput {
    fn new(name: &'static str, value: &str) -> Self {
        Self {
            name,
            value: value.into(),
            input: value.into(),
            input_mode: InputMode::Normal,
        }
    }

    fn apply_value(&mut self) {
        self.value = self.input.value().into();
    }
}

struct AppState {
    parameters: [ParameterInput; 1],
}

impl AppState {
    fn new() -> Self {
        Self {
            parameters: [ParameterInput::new("accel", "0.3")],
        }
    }
}

pub fn run_tui() -> anyhow::Result<()> {
    stdout().execute(EnterAlternateScreen)?;
    enable_raw_mode()?;
    let mut terminal = Terminal::new(CrosstermBackend::new(stdout()))?;
    terminal.clear()?;

    let mut app = AppState::new();

    loop {
        terminal.draw(|f| ui(f, &mut app))?;

        if event::poll(std::time::Duration::from_millis(16))? {
            if let event::Event::Key(key) = event::read()? {
                if key.kind == KeyEventKind::Press && key.code == KeyCode::Char('q') {
                    break;
                }

                let param = &mut app.parameters[0];

                match param.input_mode {
                    InputMode::Normal => match key.code {
                        KeyCode::Char('i') => {
                            param.input_mode = InputMode::Editing;
                        }
                        KeyCode::Char('q') => {
                            return Ok(());
                        }
                        _ => {}
                    },
                    InputMode::Editing => match key.code {
                        KeyCode::Enter => {
                            param.apply_value();
                            param.input_mode = InputMode::Normal;
                        }
                        KeyCode::Esc => {
                            param.input_mode = InputMode::Normal;
                            param.input = param.value.clone().into();
                        }
                        _ => {
                            param.input.handle_event(&Event::Key(key));
                        }
                    },
                }
            }
        }
    }

    stdout().execute(LeaveAlternateScreen)?;
    disable_raw_mode()?;
    Ok(())
}

fn ui(frame: &mut Frame, app: &mut AppState) {
    let root_layout = Layout::new(
        Direction::Vertical,
        [Constraint::Length(1), Constraint::Min(5)],
    )
    .split(frame.size());

    frame.render_widget(
        Paragraph::new(Text::from(vec![Line::from(vec![
            "maccel".blue(),
            " (press 'q' to quit)".into(),
        ])])),
        root_layout[0],
    );

    let main_layout = Layout::new(
        Direction::Horizontal,
        [Constraint::Percentage(25), Constraint::Percentage(75)],
    )
    .split(root_layout[1]);

    frame.render_widget(
        Block::default().borders(Borders::ALL).title("parameters"),
        main_layout[0],
    );

    frame.render_widget(
        Block::default().borders(Borders::ALL).title("graph"),
        main_layout[1],
    );

    // Done with main layout, now to layout the parameters inputs

    let params_layout = Layout::new(
        Direction::Vertical,
        [Constraint::Length(3), Constraint::default()],
    )
    .margin(2)
    .split(main_layout[0]);

    for param in &app.parameters {
        let input_width = params_layout[0].width.max(3) - 3; // keep 2 for borders and 1 for cursor
        let input_scroll_position = param.input.visual_scroll(input_width as usize);

        let input = Paragraph::new(param.input.value())
            .style(match param.input_mode {
                InputMode::Normal => ratatui::style::Style::default(),
                InputMode::Editing => {
                    ratatui::style::Style::default().fg(ratatui::style::Color::Yellow)
                }
            })
            .scroll((0, input_scroll_position as u16))
            .block(Block::default().borders(Borders::ALL).title(param.name));

        match param.input_mode {
            InputMode::Normal =>
                // Hide the cursor. `Frame` does this by default, so we don't need to do anything here
                {}

            InputMode::Editing => {
                // Make the cursor visible and ask tui-rs to put it at the specified coordinates after rendering
                frame.set_cursor(
                    // Put cursor past the end of the input text
                    params_layout[0].x
                        + ((param.input.visual_cursor()).max(input_scroll_position)
                            - input_scroll_position) as u16
                        + 1,
                    // Move one line down, from the border to the input line
                    params_layout[0].y + 1,
                )
            }
        }

        frame.render_widget(input, params_layout[0]);
    }
}
