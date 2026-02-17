use maccel_core::AccelMode;

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
    ScrollDown,
    ScrollUp,
}

pub type Actions = Vec<Action>;
