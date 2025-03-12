use std::fmt::Debug;

use anyhow::Context;
use maccel_core::persist::ParamStore;
use ratatui::crossterm::event::{KeyCode, KeyEvent};
use ratatui::layout::Rect;
use ratatui::{prelude::*, widgets::*};
use tui_input::Input;
use tui_input::backend::crossterm::EventHandler;

use crate::action::{Action, Actions, InputAction};
use crate::component::TuiComponent;
use maccel_core::{ContextRef, Param, Parameter};

#[derive(Debug, PartialEq)]
pub enum InputMode {
    Normal,
    Editing,
}

#[derive(Debug)]
pub struct ParameterInput<PS: ParamStore> {
    context: ContextRef<PS>,
    param_tag: Param,
    input: Input,
    pub input_mode: InputMode,
    error: Option<String>,
    pub is_selected: bool,
}

impl<PS: ParamStore> ParameterInput<PS> {
    pub fn new(param: &Parameter, context: ContextRef<PS>) -> Self {
        Self {
            context,
            param_tag: param.tag,
            input_mode: InputMode::Normal,
            input: format!("{}", param.value).into(),
            error: None,
            is_selected: false,
        }
    }

    fn this_param(&self) -> Parameter {
        *self
            .context
            .get()
            .parameter(self.param_tag)
            .expect("Failed to get param from context")
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
                self.context
                    .get_mut()
                    .update_param_value(self.param_tag, value)
            });

        match value {
            Ok(_) => {
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
        self.input = format!("{}", self.this_param().value).into();
        self.input_mode = InputMode::Normal;
    }
}

impl<PS: ParamStore + Debug> TuiComponent for ParameterInput<PS> {
    fn handle_key_event(&mut self, key: &KeyEvent, actions: &mut Actions) {
        if !self.is_selected {
            return;
        }

        let action = match self.input_mode {
            InputMode::Normal => match key.code {
                KeyCode::Char('i') | KeyCode::Enter => InputAction::Focus,
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
                    let _ = self
                        .input
                        .handle_event(&::crossterm::event::Event::Key(*key));
                    return;
                }
            },
        };

        actions.push(Action::Input(action));
    }

    fn handle_mouse_event(
        &mut self,
        _event: &::crossterm::event::MouseEvent,
        _actions: &mut Actions,
    ) {
    }

    fn update(&mut self, action: &Action) {
        if let Action::SetMode(_) = action {
            self.reset();
        }

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

    fn draw(&self, frame: &mut ratatui::Frame, area: Rect) {
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
                    .title(self.this_param().tag.display_name()),
            );

        match self.input_mode {
            InputMode::Normal =>
                // Hide the cursor. `Frame` does this by default, so we don't need to do anything here
                {}

            InputMode::Editing => {
                // Make the cursor visible and ask tui-rs to put it at the specified coordinates after rendering
                frame.set_cursor_position((
                    // Put cursor past the end of the input text
                    input_layout.x
                        + ((self.input.visual_cursor()).max(input_scroll_position)
                            - input_scroll_position) as u16
                        + 1,
                    // Move one line down, from the border to the input line
                    input_layout.y + 1,
                ))
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
