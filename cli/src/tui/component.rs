use ratatui::{layout::Rect, Frame};

use super::{
    action::{Action, Actions},
    event::{Event, KeyEvent, MouseEvent},
};

pub trait TuiComponent {
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

    fn draw(&mut self, frame: &mut Frame, area: Rect);
}
