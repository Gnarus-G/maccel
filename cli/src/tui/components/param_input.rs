use anyhow::Context;
use crossterm::event::KeyCode;
use ratatui::layout::Rect;
use ratatui::{prelude::*, widgets::*};
use tui_input::{backend::crossterm::EventHandler, Input};

use crate::params::{format_param_value, Param};
use crate::tui::action::{Action, Actions, InputAction};
use crate::tui::component::TuiComponent;
use crate::tui::context::ContextRef;
use crate::tui::event;

#[derive(Debug, PartialEq)]
pub enum InputMode {
    Normal,
    Editing,
}

pub type ParamId = usize;

#[derive(Debug)]
pub struct ParameterInput {
    context: ContextRef,
    param_id: ParamId,
    input: Input,
    pub input_mode: InputMode,
    error: Option<String>,
    pub is_selected: bool,
}

impl ParameterInput {
    pub fn new(param_id: ParamId, value: f64, context: ContextRef) -> Self {
        Self {
            context,
            param_id,
            input_mode: InputMode::Normal,
            input: format_param_value(value).into(),
            error: None,
            is_selected: false,
        }
    }

    fn param(&self) -> Param {
        self.context.get().parameters[self.param_id].param
    }

    pub fn value(&self) -> &str {
        self.input.value()
    }

    fn update_value(&mut self) {
        let value = self
            .value()
            .parse()
            .context("should be a number")
            .and_then(|value| {
                self.param().set(value)?;
                Ok(value)
            });

        match value {
            Ok(value) => {
                self.context.get_mut().parameters[self.param_id].value = value;
                self.error = None;
            }
            Err(err) => {
                self.reset();
                self.error = Some(format!("{:#}", err));
            }
        }

        self.input_mode = InputMode::Normal;
    }

    fn reset(&mut self) {
        let value = self.context.get().parameters[self.param_id].value;
        self.input = format_param_value(value).into();
        self.input_mode = InputMode::Normal;
    }
}

impl TuiComponent for ParameterInput {
    fn handle_key_event(&mut self, key: &event::KeyEvent, actions: &mut Actions) {
        if !self.is_selected {
            return;
        }

        let action = match self.input_mode {
            InputMode::Normal => match key.code {
                KeyCode::Char('i') => InputAction::Focus,
                KeyCode::BackTab | KeyCode::Up => {
                    actions.push(Action::SelectPreviousInput);
                    return;
                }
                KeyCode::Tab | KeyCode::Down => {
                    actions.push(Action::SelectNextInput);
                    return;
                }
                _ => return,
            },
            InputMode::Editing => match key.code {
                KeyCode::Enter => InputAction::Enter,
                KeyCode::Esc => InputAction::Reset,
                _ => {
                    let _ = self.input.handle_event(&crossterm::event::Event::Key(*key));
                    return;
                }
            },
        };

        actions.push(Action::Input(action));
    }

    fn handle_mouse_event(
        &mut self,
        _event: &crossterm::event::MouseEvent,
        _actions: &mut Actions,
    ) {
    }

    fn update(&mut self, action: &Action) {
        if !self.is_selected {
            return;
        }

        if let Action::Input(action) = action {
            match action {
                InputAction::Enter => {
                    self.update_value();
                }
                InputAction::Reset => self.reset(),
                InputAction::Focus => self.input_mode = InputMode::Editing,
            };
        }
    }

    fn draw(&mut self, frame: &mut ratatui::Frame, area: Rect) {
        let input_group_layout = Layout::new(
            Direction::Vertical,
            [Constraint::Min(0), Constraint::Length(2)],
        )
        .split(area);

        let input_layout = input_group_layout[0];

        let input_width = area.width.max(3) - 3; // keep 2 for borders and 1 for cursor
        let input_scroll_position = self.input.visual_scroll(input_width as usize);

        let mut input = Paragraph::new(self.input.value())
            .style(match self.input_mode {
                InputMode::Normal => ratatui::style::Style::default(),
                InputMode::Editing => {
                    ratatui::style::Style::default().fg(ratatui::style::Color::Yellow)
                }
            })
            .scroll((0, input_scroll_position as u16))
            .block(
                Block::default()
                    .borders(Borders::ALL)
                    .title(self.param().display_name()),
            );

        match self.input_mode {
            InputMode::Normal =>
                // Hide the cursor. `Frame` does this by default, so we don't need to do anything here
                {}

            InputMode::Editing => {
                // Make the cursor visible and ask tui-rs to put it at the specified coordinates after rendering
                frame.set_cursor(
                    // Put cursor past the end of the input text
                    input_layout.x
                        + ((self.input.visual_cursor()).max(input_scroll_position)
                            - input_scroll_position) as u16
                        + 1,
                    // Move one line down, from the border to the input line
                    input_layout.y + 1,
                )
            }
        }

        let helpher_text_layout = input_group_layout[1];

        if let Some(error) = &self.error {
            let helper_text = Paragraph::new(error.as_str())
                .red()
                .wrap(ratatui::widgets::Wrap { trim: true });

            frame.render_widget(helper_text, helpher_text_layout);

            input = input.red();
        }

        if self.is_selected {
            input = input.bold();
        }

        frame.render_widget(input, input_layout);
    }
}
