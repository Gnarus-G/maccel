use anyhow::Context;
use crossterm::{
    event::{self, Event, KeyCode, KeyEventKind},
    terminal::{disable_raw_mode, enable_raw_mode, EnterAlternateScreen, LeaveAlternateScreen},
    ExecutableCommand,
};
use ratatui::{
    layout::{Constraint, Direction, Layout},
    prelude::{CrosstermBackend, Stylize, Terminal},
    style::Style,
    symbols,
    text::{Line, Span, Text},
    widgets::{Axis, Block, Borders, Chart, Dataset, GraphType, Paragraph},
    Frame,
};
use std::io::stdout;
use tui_input::{backend::crossterm::EventHandler, Input};

use crate::{
    inputspeed,
    libmaccel::{sensitivity, Params},
    params::Param,
};

#[derive(PartialEq)]
enum InputMode {
    Normal,
    Editing,
}

struct ParameterInput {
    param: Param,
    value: f64,
    input: Input,
    input_mode: InputMode,
    error: Option<String>,
}

impl From<Param> for ParameterInput {
    fn from(param: Param) -> Self {
        let value: f32 = param
            .get()
            .expect("failed to read and initialize a parameter's value")
            .into();

        Self {
            param,
            value: value as f64,
            input: value.to_string().into(),
            input_mode: InputMode::Normal,
            error: None,
        }
    }
}

impl ParameterInput {
    fn update_value(&mut self) {
        let value = self
            .input
            .value()
            .parse()
            .context("should be a number")
            .and_then(|value| {
                self.param.set(value)?;
                Ok(value)
            });

        match value {
            Ok(value) => {
                self.value = value as f64;
                self.error = None;
            }
            Err(err) => {
                self.reset();
                self.error = Some(format!("{:#}", err));
            }
        }
    }

    fn reset(&mut self) {
        self.input = self.value.to_string().into();
    }
}

struct AppState {
    tab_tick: u8,
    parameters: [ParameterInput; 8],
}

impl AppState {
    fn new() -> Self {
        Self {
            tab_tick: 0,
            parameters: [
                Param::SensMult.into(),
                Param::Mode.into(),
                Param::Accel.into(),
                Param::Motivity.into(),
                Param::Gamma.into(),
                Param::SyncSpeed.into(),
                Param::Offset.into(),
                Param::OutputCap.into(),
            ],
        }
    }

    fn selected_parameter_index(&self) -> usize {
        return self.tab_tick as usize % self.parameters.len();
    }

    fn selected_parameter_input(&mut self) -> &mut ParameterInput {
        let param = &mut self.parameters[self.selected_parameter_index()];
        return param;
    }
}

pub fn run_tui() -> anyhow::Result<()> {
    stdout().execute(EnterAlternateScreen)?;
    enable_raw_mode()?;
    let mut terminal = Terminal::new(CrosstermBackend::new(stdout()))?;
    terminal.clear()?;

    let mut app = AppState::new();

    inputspeed::setup_input_speed_reader();

    loop {
        terminal.draw(|f| ui(f, &mut app))?;

        // Update procedure per frame
        if event::poll(std::time::Duration::from_millis(16))? {
            if let event::Event::Key(key) = event::read()? {
                if key.kind == KeyEventKind::Press && key.code == KeyCode::Char('q') {
                    break;
                }

                let param = app.selected_parameter_input();

                match param.input_mode {
                    InputMode::Normal => match key.code {
                        KeyCode::Char('i') => {
                            param.input_mode = InputMode::Editing;
                        }
                        KeyCode::BackTab | KeyCode::Up => {
                            app.tab_tick = app.tab_tick.wrapping_sub(1);
                        }
                        KeyCode::Tab | KeyCode::Down => {
                            app.tab_tick = app.tab_tick.wrapping_add(1);
                        }
                        _ => {}
                    },
                    InputMode::Editing => match key.code {
                        KeyCode::Enter => {
                            param.update_value();
                            param.input_mode = InputMode::Normal;
                        }
                        KeyCode::Esc => {
                            param.input_mode = InputMode::Normal;
                            param.reset();
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
        [
            Constraint::Length(1),
            Constraint::Min(5),
            Constraint::Length(3),
        ],
    )
    .horizontal_margin(2)
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

    // Bottom help section
    let normal_mode_commands_help = [
        ("Tab / Down", "select next parameter"),
        ("Shift + Tab / Up", "select previous parameter"),
        ("i", "start editing a parameter"),
    ]
    .into_iter()
    .flat_map(|(command, description)| {
        vec![
            command.bold(),
            ": ".into(),
            description.into(),
            ";  ".into(),
        ]
    })
    .collect::<Vec<_>>();

    let editin_mode_commands_help = [("Enter", "commit the value"), ("Esc", "cancel")]
        .into_iter()
        .flat_map(|(command, description)| {
            vec![
                command.bold(),
                ": ".into(),
                description.into(),
                ";  ".into(),
            ]
        })
        .collect::<Vec<_>>();

    let help = match app
        .parameters
        .iter()
        .find(|p| p.input_mode == InputMode::Editing)
    {
        Some(_) => Text::from(Line::from(editin_mode_commands_help)),
        None => Text::from(Line::from(normal_mode_commands_help)),
    };

    frame.render_widget(
        Paragraph::new(help).block(Block::new().borders(Borders::ALL)),
        root_layout[2],
    );

    // Done with main layout, now to layout the parameters inputs

    frame.render_widget(
        Block::default()
            .borders(Borders::ALL)
            .title("parameters")
            .border_style(Style::new().blue().bold()),
        main_layout[0],
    );

    let mut constraints: Vec<_> = app
        .parameters
        .iter()
        .map(|_| Constraint::Length(5))
        .collect();

    constraints.push(Constraint::default());
    let params_layout = Layout::new(Direction::Vertical, constraints)
        .margin(2)
        .split(main_layout[0]);

    for (idx, param) in app.parameters.iter().enumerate() {
        let input_group_layout = params_layout[idx];
        let input_group_layout = Layout::new(
            Direction::Vertical,
            [Constraint::Min(0), Constraint::Length(2)],
        )
        .split(input_group_layout);

        let input_layout = input_group_layout[0];

        let input_width = params_layout[0].width.max(3) - 3; // keep 2 for borders and 1 for cursor
        let input_scroll_position = param.input.visual_scroll(input_width as usize);

        let mut input = Paragraph::new(param.input.value())
            .style(match param.input_mode {
                InputMode::Normal => ratatui::style::Style::default(),
                InputMode::Editing => {
                    ratatui::style::Style::default().fg(ratatui::style::Color::Yellow)
                }
            })
            .scroll((0, input_scroll_position as u16))
            .block(
                Block::default()
                    .borders(Borders::ALL)
                    .title(param.param.display_name()),
            );

        match param.input_mode {
            InputMode::Normal =>
                // Hide the cursor. `Frame` does this by default, so we don't need to do anything here
                {}

            InputMode::Editing => {
                // Make the cursor visible and ask tui-rs to put it at the specified coordinates after rendering
                frame.set_cursor(
                    // Put cursor past the end of the input text
                    input_layout.x
                        + ((param.input.visual_cursor()).max(input_scroll_position)
                            - input_scroll_position) as u16
                        + 1,
                    // Move one line down, from the border to the input line
                    input_layout.y + 1,
                )
            }
        }

        let helpher_text_layout = input_group_layout[1];

        if let Some(error) = &param.error {
            let helper_text = Paragraph::new(error.as_str())
                .red()
                .wrap(ratatui::widgets::Wrap { trim: true });

            frame.render_widget(helper_text, helpher_text_layout);

            input = input.red();
        }

        if idx == app.selected_parameter_index() {
            input = input.bold();
        }

        frame.render_widget(input, input_layout);
    }

    // Done with parameter inputs, now on to the graph

    let (bounds, labels) = bounds_and_labels([0.0, 128.0], 16);
    let x_axis = Axis::default()
        .title("Speed_in".magenta())
        .style(Style::default().white())
        .bounds(bounds)
        .labels(labels);

    // To make the y axis bounds and labels scale with our sensitivity multiplier
    let auto_scaled_y_bounds = {
        let mut out_cap = app
            .parameters
            .iter()
            .find(|&p| p.param == Param::OutputCap)
            .expect("we should include the Output cap param in the list")
            .value;
        out_cap = out_cap.max(1.0);

        let sens_mult = app
            .parameters
            .iter()
            .find(|&p| p.param == Param::SensMult)
            .expect("we should include the Sensitivity multiplier param in the list")
            .value;
        [0.0, sens_mult * out_cap * 2.0]
    };

    let (bounds, labels) = bounds_and_labels(auto_scaled_y_bounds, 5);
    let y_axis = Axis::default()
        .title("Sensitivity".magenta())
        .style(Style::default().white())
        .bounds(bounds)
        .labels(labels);

    // todo: for the graph it would be cool if we don't show the output of 0, but a really small number close to zero so the function looks more continuous
    let data: Vec<_> = (0..1000)
        .map(|x| (x as f32) * 0.1375)
        .map(|x| (x as f64, sensitivity(x, Params::new())))
        .collect();

    let highlight_point = &[inputspeed::read_input_speed_and_resolved_sens()];
    let chart = Chart::new(vec![
        Dataset::default()
            .name(format!(
                "f(x) = (1 + {}⋅x) ⋅ {}",
                Param::Accel.display_name(),
                Param::SensMult.display_name()
            ))
            .marker(symbols::Marker::Braille)
            .graph_type(GraphType::Line)
            .style(Style::default().green())
            .data(&data),
        Dataset::default() // current instance of acceleration
            .marker(symbols::Marker::Braille)
            .style(Style::default().red())
            .data(highlight_point),
    ])
    .x_axis(x_axis)
    .y_axis(y_axis);

    frame.render_widget(
        chart.block(
            Block::default()
                .borders(Borders::NONE)
                .title("graph (Sensitivity = Speed_out / Speed_in)")
                .bold(),
        ),
        main_layout[1],
    );
}

fn bounds_and_labels(bounds: [f64; 2], div: usize) -> ([f64; 2], Vec<Span<'static>>) {
    let [o, f] = bounds;
    let d = f - o;
    let step = d / (div as f64);

    let labels = (0..=div)
        .map(|i| o + i as f64 * step)
        .map(|label| format!("{:.2}", label).into())
        .collect();

    return (bounds, labels);
}
