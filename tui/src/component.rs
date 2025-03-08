use std::fmt::Debug;

use crossterm::event::{KeyEvent, MouseEvent};
use ratatui::{layout::Rect, Frame};

use super::{
    action::{Action, Actions},
    event::Event,
};

pub trait TuiComponent: Debug {
    fn handle_event(&mut self, event: &Event, actions: &mut Actions) {
        match event {
            Event::Key(key_event) => self.handle_key_event(key_event, actions),
            Event::Mouse(mouse_event) => self.handle_mouse_event(mouse_event, actions),
        }
    }
    fn handle_key_event(&mut self, event: &KeyEvent, actions: &mut Actions);
    fn handle_mouse_event(&mut self, event: &MouseEvent, actions: &mut Actions);

    /// Stuff to do on any action derived from an event
    fn update(&mut self, action: &Action);

    fn draw(&self, frame: &mut Frame, area: Rect);
}

#[derive(Debug)]
pub struct NoopComponent;

impl TuiComponent for NoopComponent {
    fn handle_key_event(&mut self, _event: &KeyEvent, _actions: &mut Actions) {}

    fn handle_mouse_event(&mut self, _event: &MouseEvent, _actions: &mut Actions) {}

    fn update(&mut self, _action: &Action) {}

    fn draw(&self, _frame: &mut Frame, _area: Rect) {}
}
