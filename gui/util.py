# SPDX-License-Identifier: GPL-2.0-or-later

def hook(ui, event, callback, value = None):
    '''
    Hooks a callback to an Qty element event and passes over a value (if <value> not None)
    
    :param ui: Qty ui object
    :param event: Event we want to connect to
    :param callback: Function to be executed when the event happens. Takes 0 arguments, if <value> = None and 1 argument otherwise
    :param value: If set, the method ui.<value> will be called and the <result> passed over to action(<result>)
    :return: returns nothing
    '''
    event = getattr(ui,event)
    if(value):
        if(not hasattr(ui,value)):
            print(f"Could not hook to {type(ui)}: No such event {value}")
            return
        event.connect(lambda: callback(getattr(ui,value)()))
    else:
        event.connect(lambda: callback())