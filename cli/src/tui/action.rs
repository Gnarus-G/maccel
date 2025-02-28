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
}

pub type Actions = Vec<Action>;
