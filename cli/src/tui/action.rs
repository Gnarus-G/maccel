use super::context::AccelMode;

#[derive(Debug, PartialEq)]
pub enum InputAction {
    Enter,
    Focus,
    Reset,
}

#[derive(Debug, PartialEq)]
pub enum Action {
    Tick,
    Input(InputAction),
    SelectNextInput,
    SelectPreviousInput,
    SetMode(AccelMode),
}

pub type Actions = Vec<Action>;
