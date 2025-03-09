use std::fmt::Debug;

use crossterm::event;
use maccel_core::persist::ParamStore;
use ratatui::layout::Rect;
use ratatui::{prelude::*, widgets::*};

use crate::action::{Action, Actions};
use crate::component::TuiComponent;
use crate::param_input::ParameterInput;
use crate::utils::CyclingIdx;
use maccel_core::AccelMode;

use super::param_input::InputMode;

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
        };

        s.parameters[0].is_selected = true;

        s
    }

    fn selected_parameter_index(&self) -> usize {
        self.param_idx.current()
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

    fn handle_mouse_event(
        &mut self,
        _event: &crossterm::event::MouseEvent,
        _actions: &mut Actions,
    ) {
    }

    fn update(&mut self, action: &Action) {
        match action {
            Action::SelectNextInput => {
                self.param_idx.forward();
            }
            Action::SelectPreviousInput => {
                self.param_idx.back();
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

        // Bottom help section
        let normal_mode_commands_help = [
            ("Tab / Down", "select next parameter"),
            ("Shift + Tab / Up", "select previous parameter"),
            ("i", "start editing a parameter"),
            ("Left", "prev mode"),
            ("Right", "next mode"),
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

        let help = match self.help_text_mode() {
            HelpTextMode::EditMode => Text::from(Line::from(editin_mode_commands_help)),
            HelpTextMode::NormalMode => Text::from(Line::from(normal_mode_commands_help)),
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

        let mut constraints: Vec<_> = self
            .parameters
            .iter()
            .map(|_| Constraint::Length(5))
            .collect();

        constraints.push(Constraint::default());
        let params_layout = Layout::new(Direction::Vertical, constraints)
            .margin(2)
            .split(main_layout[0]);

        for (idx, param) in self.parameters.iter().enumerate() {
            param.draw(frame, params_layout[idx]);
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
