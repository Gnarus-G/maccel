use ratatui::layout::Rect;
use ratatui::{prelude::*, widgets::*};

use crate::tui::action::{Action, Actions};
use crate::tui::component::TuiComponent;
use crate::tui::context::ContextRef;
use crate::tui::event;

use super::graph::Graph;
use super::param_input::InputMode;
use super::ParameterInput;

#[derive(Debug, Default, Clone, Copy, PartialEq)]
pub enum HelpTextMode {
    EditMode,
    #[default]
    NormalMode,
}

#[derive(Debug)]
pub struct Screen {
    context: ContextRef,
    tab_tick: u8,
    parameters: Vec<ParameterInput>,
    graph: Graph,
}

impl Screen {
    pub fn new(context: ContextRef) -> Self {
        let mut s = Self {
            tab_tick: 0,
            parameters: context
                .clone()
                .get()
                .parameters
                .iter()
                .enumerate()
                .map(|(id, p)| ParameterInput::new(id, p.value, context.clone()))
                .collect(),
            graph: Graph::new(context.clone()),
            context,
        };

        s.parameters[0].is_selected = true;

        s
    }

    fn selected_parameter_index(&self) -> usize {
        self.tab_tick as usize % self.parameters.len()
    }

    fn help_text_mode(&self) -> HelpTextMode {
        let is_in_editing_mode = self
            .parameters
            .iter()
            .any(|p| p.input_mode == InputMode::Editing);

        if is_in_editing_mode {
            HelpTextMode::EditMode
        } else {
            HelpTextMode::NormalMode
        }
    }
}

impl TuiComponent for Screen {
    fn handle_key_event(&mut self, event: &event::KeyEvent, actions: &mut Actions) {
        let selected_param_idx = self.selected_parameter_index();
        for (idx, param) in self.parameters.iter_mut().enumerate() {
            param.is_selected = selected_param_idx == idx;
            param.handle_key_event(event, actions);
        }
        self.graph.handle_key_event(event, actions);
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
                self.tab_tick = self.tab_tick.wrapping_add(1);
            }
            Action::SelectPreviousInput => {
                self.tab_tick = self.tab_tick.wrapping_sub(1);
            }
            _ => {}
        }

        let selected_param_idx = self.selected_parameter_index();
        for (idx, param) in self.parameters.iter_mut().enumerate() {
            param.is_selected = selected_param_idx == idx;
            param.update(action);
        }
        self.graph.update(action);
    }

    fn draw(&mut self, frame: &mut ratatui::Frame, area: Rect) {
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
                .title("parameters")
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

        for (idx, param) in self.parameters.iter_mut().enumerate() {
            param.draw(frame, params_layout[idx]);
        }
        // Done with parameter inputs, now on to the graph

        self.graph.draw(frame, main_layout[1]);
    }
}

#[cfg(test)]
mod test {
    use crossterm::event::KeyCode;

    use crate::tui::{
        action::Action,
        component::TuiComponent,
        components::HelpTextMode,
        context::{ContextRef, Parameter},
    };

    use super::Screen;

    #[test]
    fn can_select_input() {
        let context = ContextRef::new(crate::tui::context::Context {
            parameters: vec![
                Parameter::new(crate::params::Param::SensMult, 1.0),
                Parameter::new(crate::params::Param::Accel, 1.0),
            ],
        });

        let mut screen = Screen::new(context);
        assert_eq!(screen.selected_parameter_index(), 0);
        assert!(screen.parameters[0].is_selected);

        let mut actions = vec![];

        screen.handle_event(
            &crate::tui::event::Event::Key(KeyCode::Down.into()),
            &mut actions,
        );

        assert_eq!(actions, vec![Action::SelectNextInput]);
        assert_eq!(screen.selected_parameter_index(), 0);
        assert!(screen.parameters[0].is_selected);

        screen.update(&actions[0]);
        assert_eq!(screen.selected_parameter_index(), 1);
        assert!(screen.parameters[1].is_selected);

        actions.clear();

        screen.handle_event(
            &crate::tui::event::Event::Key(KeyCode::Up.into()),
            &mut actions,
        );

        assert_eq!(actions, vec![Action::SelectPreviousInput]);

        screen.update(&actions[0]);
        assert_eq!(screen.selected_parameter_index(), 0);
        assert!(screen.parameters[0].is_selected);
    }

    #[test]
    fn can_show_edit_mode_help_text() {
        let context = ContextRef::new(crate::tui::context::Context {
            parameters: vec![
                Parameter::new(crate::params::Param::SensMult, 1.0),
                Parameter::new(crate::params::Param::SensMult, 1.0),
            ],
        });

        let mut screen = Screen::new(context);

        assert_eq!(screen.help_text_mode(), HelpTextMode::NormalMode);

        let mut actions = vec![];
        screen.handle_event(
            &crate::tui::event::Event::Key(KeyCode::Char('i').into()),
            &mut actions,
        );

        assert_eq!(
            actions,
            vec![Action::Input(crate::tui::action::InputAction::Focus)]
        );

        screen.update(&actions[0]);
        assert_eq!(screen.help_text_mode(), HelpTextMode::EditMode);
    }

    #[test]
    fn can_edit_input() {
        let context = ContextRef::new(crate::tui::context::Context {
            parameters: vec![
                Parameter::new(crate::params::Param::SensMult, 1.0),
                Parameter::new(crate::params::Param::Accel, 0.0),
            ],
        });

        let mut screen = Screen::new(context);
        let mut actions = vec![];

        screen.handle_event(
            &crate::tui::event::Event::Key(KeyCode::Char('i').into()),
            &mut actions,
        );
        assert_eq!(
            actions,
            vec![Action::Input(crate::tui::action::InputAction::Focus)]
        );
        screen.update(&actions[0]);
        actions.clear();

        screen.handle_event(
            &crate::tui::event::Event::Key(KeyCode::Char('1').into()),
            &mut actions,
        );
        assert_eq!(actions, vec![]);

        assert_eq!(
            screen.parameters[screen.selected_parameter_index()].value(),
            "1.000001"
        );

        screen.handle_event(
            &crate::tui::event::Event::Key(KeyCode::Char('9').into()),
            &mut actions,
        );

        assert_eq!(
            screen.parameters[screen.selected_parameter_index()].value(),
            "1.0000019"
        );
    }
}
