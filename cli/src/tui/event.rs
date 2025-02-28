use std::sync::mpsc::TryRecvError;

use crossterm::event::Event as CrosstermEvent;

pub use crossterm::event::KeyEvent;
pub use crossterm::event::MouseEvent;
use crossterm::event::MouseEventKind;

#[derive(Debug)]
pub enum Event {
    Key(KeyEvent),
    Mouse(MouseEvent),
}

#[derive(Debug)]
pub struct EventHandler {
    rx: std::sync::mpsc::Receiver<Event>,
}

impl EventHandler {
    pub fn new() -> Self {
        let tick_rate = std::time::Duration::from_millis(250);
        let (tx, rx) = std::sync::mpsc::channel();
        std::thread::spawn(move || -> anyhow::Result<()> {
            loop {
                if crossterm::event::poll(tick_rate)? {
                    match crossterm::event::read()? {
                        CrosstermEvent::Key(e) => tx.send(Event::Key(e)),
                        CrosstermEvent::Mouse(e) => {
                            if e.kind == MouseEventKind::Moved {
                                // These are annoying because they get buffered
                                // while you move your mouse a bunch and delay
                                // other events, e.g 'q' quitting could much longer than it should.
                                continue;
                            }
                            tx.send(Event::Mouse(e))
                        }
                        CrosstermEvent::Resize(_col, _row) => continue,
                        event => unimplemented!("event {event:?}"),
                    }?
                }
            }
        });

        EventHandler { rx }
    }

    pub fn next(&self) -> anyhow::Result<Option<Event>> {
        Ok(match self.rx.try_recv() {
            Ok(e) => Some(e),
            Err(TryRecvError::Empty) => None,
            Err(err) => return Err(err.into()),
        })
    }
}
