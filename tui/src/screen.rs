use std::cell::Cell;

use std::fmt::Debug;

use crossterm::event;
use maccel_core::persist::ParamStore;
use ratatui::layout::Rect;
use ratatui::{prelude::*, widgets::*};

use crate::action::{Action, Actions};
use crate::component::TuiComponent;
use crate::param_input::ParameterInput;
use crate::utils::CyclingIdx;
use crate::widgets::scrollbar::CustomScrollbar;
use maccel_core::AccelMode;

use super::param_input::InputMode;

const PARAM_HEIGHT: u16 = 5;

#[derive(Debug, Default, Clone, Copy, PartialEq)]
pub enum HelpTextMode {
    EditMode,
    #[default]
    NormalMode,
}

pub struct Screen<PS: ParamStore> {
    pub accel_mode: AccelMode,
    param_idx: CyclingIdx,
    parameters: Vec<ParameterInput<PS>>,
    preview_slot: Box<dyn TuiComponent>,
    scroll_offset: usize,
    visible_count: Cell<usize>,
}

impl<PS: ParamStore> Screen<PS> {
    pub fn new(
        mode: AccelMode,
        parameters: Vec<ParameterInput<PS>>,
        preview: Box<dyn TuiComponent>,
    ) -> Self {
        let mut s = Self {
            param_idx: CyclingIdx::new(parameters.len()),
            accel_mode: mode,
            parameters,
            preview_slot: preview,
            scroll_offset: 0,
            visible_count: Cell::new(0),
        };

        s.parameters[0].is_selected = true;

        s
    }

    fn selected_parameter_index(&self) -> usize {
        self.param_idx.current()
    }

    fn scroll_to_selected(&mut self) {
        let visible = self.visible_count.get().max(1);
        let selected = self.selected_parameter_index();

        if selected < self.scroll_offset {
            self.scroll_offset = selected;
        } else if visible > 0 && selected >= self.scroll_offset + visible {
            self.scroll_offset = selected - visible + 1;
        }
    }

    pub fn is_in_editing_mode(&self) -> bool {
        self.parameters
            .iter()
            .any(|p| p.input_mode == InputMode::Editing)
    }

    fn help_text_mode(&self) -> HelpTextMode {
        if self.is_in_editing_mode() {
            HelpTextMode::EditMode
        } else {
            HelpTextMode::NormalMode
        }
    }
}

impl<PS: ParamStore + Debug> TuiComponent for Screen<PS> {
    fn handle_key_event(&mut self, event: &event::KeyEvent, actions: &mut Actions) {
        let selected_param_idx = self.selected_parameter_index();
        for (idx, param) in self.parameters.iter_mut().enumerate() {
            param.is_selected = selected_param_idx == idx;
            param.handle_key_event(event, actions);
        }
        self.preview_slot.handle_key_event(event, actions);
    }

    fn handle_mouse_event(&mut self, event: &crossterm::event::MouseEvent, actions: &mut Actions) {
        use crossterm::event::MouseEventKind;
        match event.kind {
            MouseEventKind::ScrollDown => actions.push(Action::ScrollDown),
            MouseEventKind::ScrollUp => actions.push(Action::ScrollUp),
            _ => {}
        }
    }

    fn update(&mut self, action: &Action) {
        match action {
            Action::SelectNextInput => {
                self.param_idx.forward();
                self.scroll_to_selected();
            }
            Action::SelectPreviousInput => {
                self.param_idx.back();
                self.scroll_to_selected();
            }
            Action::ScrollDown => {
                let max_scroll = self.parameters.len().saturating_sub(1);
                if self.scroll_offset < max_scroll {
                    self.scroll_offset += 1;
                }
            }
            Action::ScrollUp => {
                if self.scroll_offset > 0 {
                    self.scroll_offset -= 1;
                }
            }
            Action::ScrollPageDown => {
                let visible = self.visible_count.get().max(1);
                let max_scroll = self.parameters.len().saturating_sub(visible);
                self.scroll_offset = (self.scroll_offset + visible).min(max_scroll);
            }
            Action::ScrollPageUp => {
                let visible = self.visible_count.get().max(1);
                self.scroll_offset = self.scroll_offset.saturating_sub(visible);
            }
            _ => {}
        }

        let selected_param_idx = self.selected_parameter_index();
        // debug!(
        //     "selected parameter {} on current screen, tick = {}",
        //     selected_param_idx, self.param_idx
        // );
        for (idx, param) in self.parameters.iter_mut().enumerate() {
            param.is_selected = selected_param_idx == idx;
            param.update(action);
        }
        self.preview_slot.update(action);
    }

    fn draw(&self, frame: &mut ratatui::Frame, area: Rect) {
        let root_layout = Layout::new(
            Direction::Vertical,
            [
                Constraint::Length(1),
                Constraint::Min(5),
                Constraint::Length(3),
            ],
        )
        .horizontal_margin(2)
        .split(area);

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

        let selected_param = &self.parameters[self.selected_parameter_index()];
        let description = selected_param.param().description();

        let help = match self.help_text_mode() {
            HelpTextMode::EditMode => {
                let commands: Vec<Span> = [
                    ("Enter", "commit"),
                    ("Esc", "cancel"),
                    ("Left/Right", "change mode"),
                ]
                .into_iter()
                .flat_map(|(cmd, desc)| vec![cmd.bold(), ": ".into(), desc.into(), "  ".into()])
                .collect();
                Text::from(Line::from(commands))
            }
            HelpTextMode::NormalMode => {
                let commands: Vec<Span> = [
                    ("Tab/Up/Down", "navigate"),
                    ("i/Enter", "edit"),
                    ("Left/Right", "mode"),
                ]
                .into_iter()
                .flat_map(|(cmd, desc)| vec![cmd.bold(), " ".into(), desc.into(), "  ".into()])
                .collect();
                let mut lines = vec![Line::from(commands)];
                lines.push(Line::from(""));
                lines.push(Line::from(description.italic()));
                Text::from(lines)
            }
        };

        frame.render_widget(
            Paragraph::new(help).block(Block::new().borders(Borders::ALL)),
            root_layout[2],
        );

        // Done with main layout, now to layout the parameters inputs

        frame.render_widget(
            Block::default()
                .borders(Borders::ALL)
                .title(self.accel_mode.as_title())
                .border_style(Style::new().blue().bold()),
            main_layout[0],
        );

        let params_area = main_layout[0].inner(Margin {
            vertical: 1,
            horizontal: 1,
        });

        let available_height = params_area.height as usize;
        let param_count = self.parameters.len();
        let params_fit = param_count * (PARAM_HEIGHT as usize) <= available_height;

        if params_fit || param_count == 0 {
            let constraints: Vec<_> = self
                .parameters
                .iter()
                .map(|_| Constraint::Length(PARAM_HEIGHT))
                .collect();

            let params_layout = Layout::new(Direction::Vertical, constraints)
                .margin(1)
                .split(params_area);

            for (idx, param) in self.parameters.iter().enumerate() {
                param.draw(frame, params_layout[idx]);
            }
        } else {
            let margin_height = 2;
            let content_height = available_height.saturating_sub(margin_height);
            let visible = content_height / (PARAM_HEIGHT as usize);
            self.visible_count.set(visible);
            let start = self
                .scroll_offset
                .min(param_count.saturating_sub(visible.max(1)));
            let end = (start + visible.max(1)).min(param_count);

            let constraints: Vec<_> = self.parameters[start..end]
                .iter()
                .map(|_| Constraint::Length(PARAM_HEIGHT))
                .collect();

            let params_layout = Layout::new(Direction::Vertical, constraints)
                .margin(1)
                .split(params_area);

            for (i, param) in self.parameters[start..end].iter().enumerate() {
                param.draw(frame, params_layout[i]);
            }

            // Calculate scrollbar area based on rendered params
            let first_rect = params_layout.first().copied().unwrap_or(params_area);
            let last_rect = params_layout.last().copied().unwrap_or(params_area);
            let visible_items = end - start;

            // Area includes arrows above and below the content
            let scrollbar_area = Rect {
                x: params_area.x + params_area.width.saturating_sub(1),
                y: first_rect.y.saturating_sub(1),
                width: 1,
                height: (last_rect.y + last_rect.height - first_rect.y + 1) + 2,
            };

            let scrollbar = CustomScrollbar::new(param_count, visible_items, start);
            frame.render_widget(scrollbar, scrollbar_area);
        }
        // Done with parameter inputs, now on to the graph

        self.preview_slot.draw(frame, main_layout[1]);
    }
}

#[cfg(test)]
mod test {
    use maccel_core::{persist::ParamStore, AccelMode};

    use crossterm::event::KeyCode;

    use crate::{
        action::Action, component::TuiComponent, param_input::ParameterInput, screen::HelpTextMode,
        utils::test_utils::new_context,
    };

    use super::Screen;

    impl<PS: ParamStore> Screen<PS> {
        #[cfg(test)]
        pub fn with_no_preview(mode: AccelMode, parameters: Vec<ParameterInput<PS>>) -> Self {
            use crate::component::NoopComponent;
            Self::new(mode, parameters, Box::new(NoopComponent))
        }
    }

    #[test]
    fn can_select_input() {
        let (context, parameters) = new_context();
        let inputs = parameters
            .iter()
            .map(|p| ParameterInput::new(p, context.clone()))
            .collect();

        let mut screen = Screen::with_no_preview(AccelMode::Linear, inputs);
        assert_eq!(screen.selected_parameter_index(), 0);
        assert!(screen.parameters[0].is_selected);

        let mut actions = vec![];

        screen.handle_event(
            &crate::event::Event::Key(KeyCode::Down.into()),
            &mut actions,
        );

        assert_eq!(actions, vec![Action::SelectNextInput]);
        assert_eq!(screen.selected_parameter_index(), 0);
        assert!(screen.parameters[0].is_selected);

        screen.update(&actions[0]);
        assert_eq!(screen.selected_parameter_index(), 1);
        assert!(screen.parameters[1].is_selected);

        actions.clear();

        screen.handle_event(&crate::event::Event::Key(KeyCode::Up.into()), &mut actions);

        assert_eq!(actions, vec![Action::SelectPreviousInput]);

        screen.update(&actions[0]);
        assert_eq!(screen.selected_parameter_index(), 0);
        assert!(screen.parameters[0].is_selected);
    }

    #[test]
    fn can_show_edit_mode_help_text() {
        let (context, parameters) = new_context();

        let inputs = parameters
            .iter()
            .map(|p| ParameterInput::new(p, context.clone()))
            .collect();

        let mut screen = Screen::with_no_preview(AccelMode::Linear, inputs);

        assert_eq!(screen.help_text_mode(), HelpTextMode::NormalMode);

        let mut actions = vec![];
        screen.handle_event(
            &crate::event::Event::Key(KeyCode::Char('i').into()),
            &mut actions,
        );

        assert_eq!(
            actions,
            vec![Action::Input(crate::action::InputAction::Focus)]
        );

        screen.update(&actions[0]);
        assert_eq!(screen.help_text_mode(), HelpTextMode::EditMode);
    }

    #[test]
    fn can_edit_input() {
        let (context, parameters) = new_context();

        let inputs = parameters
            .iter()
            .map(|p| ParameterInput::new(p, context.clone()))
            .collect();

        let mut screen = Screen::with_no_preview(AccelMode::Linear, inputs);
        let mut actions = vec![];

        screen.handle_event(
            &crate::event::Event::Key(KeyCode::Char('i').into()),
            &mut actions,
        );
        assert_eq!(
            actions,
            vec![Action::Input(crate::action::InputAction::Focus)]
        );
        screen.update(&actions[0]);
        actions.clear();

        screen.handle_event(
            &crate::event::Event::Key(KeyCode::Char('1').into()),
            &mut actions,
        );
        assert_eq!(actions, vec![]);

        assert_eq!(
            screen.parameters[screen.selected_parameter_index()].value(),
            "11" // 0's are to be trimmed from the right
        );

        screen.handle_event(
            &crate::event::Event::Key(KeyCode::Char('9').into()),
            &mut actions,
        );

        assert_eq!(
            screen.parameters[screen.selected_parameter_index()].value(),
            "119"
        );
    }
}
