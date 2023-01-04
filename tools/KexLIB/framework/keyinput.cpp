// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2012 Samuel Villarreal
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION: Key input handling and binding
//
//-----------------------------------------------------------------------------

#include "kexlib.h"

static kexInputKey inputKey;
kexInputKey *kexlib::inputBinds = &inputKey;

typedef struct {
    int         code;
    const char  *name;
} keyinfo_t;

static keyinfo_t Keys[] = {
    { KM_BUTTON_LEFT,       "mouse1" },
    { KM_BUTTON_MIDDLE,     "mouse2" },
    { KM_BUTTON_RIGHT,      "mouse3" },
    { KKEY_RIGHT,           "right" },
    { KKEY_LEFT,            "left" },
    { KKEY_UP,              "up" },
    { KKEY_DOWN,            "down" },
    { KKEY_ESCAPE,          "escape" },
    { KKEY_RETURN,          "enter" },
    { KKEY_TAB,             "tab" },
    { KKEY_BACKSPACE,       "backsp" },
    { KKEY_PAUSE,           "pause" },
    { KKEY_LSHIFT,          "shift" },
    { KKEY_LALT,            "alt" },
    { KKEY_LCTRL,           "ctrl" },
    { KKEY_PLUS,            "+" },
    { KKEY_MINUS,           "-" },
    { KKEY_CAPSLOCK,        "caps" },
    { KKEY_INSERT,          "ins" },
    { KKEY_DELETE,          "del" },
    { KKEY_HOME,            "home" },
    { KKEY_END,             "end" },
    { KKEY_PAGEUP,          "pgup" },
    { KKEY_PAGEDOWN,        "pgdn" },
    { KKEY_SPACE,           "space" },
    { KKEY_F1,              "f1" },
    { KKEY_F2,              "f2" },
    { KKEY_F3,              "f3" },
    { KKEY_F4,              "f4" },
    { KKEY_F5,              "f5" },
    { KKEY_F6,              "f6" },
    { KKEY_F7,              "f7" },
    { KKEY_F8,              "f8" },
    { KKEY_F9,              "f9" },
    { KKEY_F10,             "f10" },
    { KKEY_F11,             "f11" },
    { KKEY_F12,             "f12" },
    { KKEY_KP_ENTER,        "keypadenter" },
    { KKEY_KP_MULTIPLY,     "keypad*" },
    { KKEY_KP_PLUS,         "keypad+" },
    { KKEY_NUMLOCKCLEAR,    "numlock" },
    { KKEY_KP_MINUS,        "keypad-" },
    { KKEY_KP_PERIOD,       "keypad." },
    { KKEY_KP_DIVIDE,       "keypad/" },
    { KKEY_KP_0,            "keypad0" },
    { KKEY_KP_1,            "keypad1" },
    { KKEY_KP_2,            "keypad2" },
    { KKEY_KP_3,            "keypad3" },
    { KKEY_KP_4,            "keypad4" },
    { KKEY_KP_5,            "keypad5" },
    { KKEY_KP_6,            "keypad6" },
    { KKEY_KP_7,            "keypad7" },
    { KKEY_KP_8,            "keypad8" },
    { KKEY_KP_9,            "keypad9" },
    { 0,                    NULL }
};

//
// FCmd_KeyAction
//

static void FCmd_KeyAction(void) {
    char *argv;
    int action;

    argv = kexlib::commands->GetArgv(0);
    action = inputKey.FindAction(argv);

    if(action == -1) {
        return;
    }

    if(argv[0] == '-') {
        action |= CKF_UP;
    }

    inputKey.HandleControl(action);
}

//
// bind
//

COMMAND(bind) {
    int argc;
    int key;
    int i;
    char cmd[1024];

    argc = kexlib::commands->GetArgc();

    if(argc < 3) {
        kexlib::system->Printf("bind <key> <command>\n");
        return;
    }

    if(!(key = inputKey.GetKeyCode(kexlib::commands->GetArgv(1)))) {
        kexlib::system->Warning("\"%s\" isn't a valid key\n", kexlib::commands->GetArgv(1));
        return;
    }

    cmd[0] = 0;
    for(i = 2; i < argc; i++) {
        strcat(cmd, kexlib::commands->GetArgv(i));
        if(i != (argc - 1)) {
            strcat(cmd, " ");
        }
    }

    inputKey.BindCommand(key, cmd);
}

//
// unbind
//

COMMAND(unbind) {
}

//
// listbinds
//

COMMAND(listbinds) {
    inputKey.ListBindings();
}

//
// kexInputKey::GetKeyCode
//

int kexInputKey::GetKeyCode(char *key) {
    keyinfo_t *pkey;
    int len;
    kexStr tmp(key);

    tmp.ToLower();
    len = tmp.Length();

    if(len == 1) {
        if(((*key >= 'a') && (*key <= 'z')) || ((*key >= '0') && (*key <= '9'))) {
            return *key;
        }
    }

    for(pkey = Keys; pkey->name; pkey++) {
        if(!strcmp(key, pkey->name)) {
            return pkey->code;
        }
    }

    return 0;
}

//
// kexInputKey::GetName
//

bool kexInputKey::GetName(char *buff, int key) {
    keyinfo_t *pkey;
        
    if(((key >= 'a') && (key <= 'z')) || ((key >= '0') && (key <= '9'))) {
        buff[0] = (char)toupper(key);
        buff[1] = 0;

        return true;
    }
    
    for(pkey = Keys; pkey->name; pkey++) {
        int keycode = pkey->code;
        keycode &= ~KKEY_SCANCODE_MASK;
        
        if(keycode == key) {
            strcpy(buff, pkey->name);
            return true;
        }
    }
    sprintf(buff, "Key%02x", key);
    return false;
}

//
// kexInputKey::BindCommand
//

void kexInputKey::BindCommand(char key, const char *string) {
    keycmd_t *keycmd;
    cmdlist_t *newcmd;

    keycmd = &keycmds[keycode[bShiftdown][key]];
    newcmd = (cmdlist_t*)Mem_Malloc(sizeof(cmdlist_t), hb_static);
    newcmd->command = Mem_Strdup(string, hb_static);
    newcmd->next = keycmd->cmds;
    keycmd->cmds = newcmd;
}

//
// kexInputKey::Clear
//

void kexInputKey::Clear(void) {
    memset(&control, 0, sizeof(control_t));
}

//
// kexInputKey::HandleControl
//

void kexInputKey::HandleControl(int ctrl) {
    int ctrlkey;
    
    ctrlkey = (ctrl & CKF_COUNTMASK);
    
    if(ctrl & CKF_UP) {
        if((control.actions[ctrlkey] & CKF_COUNTMASK) > 0)
            control.actions[ctrlkey] = 0;
    }
    else {
        control.actions[ctrlkey] = 1;
    }
}

//
// kexInputKey::ListBindings
//

void kexInputKey::ListBindings(void) {
    keycmd_t *keycmd;
    cmdlist_t *cmd;
    kexStr tmp;

    kexlib::system->Printf("\n");

    for(int i = 0; i < MAX_KEYS; i++) {
        keycmd = &keycmds[i];

        for(cmd = keycmd->cmds; cmd; cmd = cmd->next) {
            char buff[32];

            GetName(buff, i);
            tmp = buff;
            tmp.ToLower();

            kexlib::system->CPrintf(COLOR_GREEN, "%s : \"%s\"\n", tmp.c_str(), cmd->command);
        }
    }
}

//
// kexInputKey::FindAction
//

int kexInputKey::FindAction(const char *name) {
    keyaction_t *action;
    unsigned int hash;

    if(name[0] == 0)
        return -1;

    hash = kexStr::Hash(name);

    for(action = keyactions[hash]; action; action = action->next) {
        if(!strcmp(name, action->name)) {
            return action->keyid;
        }
    }

    return -1;
}

//
// kexInputKey::ExecuteCommand
//

void kexInputKey::ExecuteCommand(int key, bool keyup) {
    keycmd_t *keycmd;
    cmdlist_t *cmd;
    
    key &= ~KKEY_SCANCODE_MASK;

    if(key >= MAX_KEYS) {
        return;
    }

    keycmd = &keycmds[keycode[bShiftdown][key]];

    for(cmd = keycmd->cmds; cmd; cmd = cmd->next) {
        if(cmd->command[0] == '+' || cmd->command[0] == '-') {
            if(keyup && cmd->command[0] == '+') {
                cmd->command[0] = '-';
            }

            kexlib::commands->Execute(cmd->command);

            if(keyup && cmd->command[0] == '-') {
                cmd->command[0] = '+';
            }
        }
        else if(!keyup) {
            kexlib::commands->Execute(cmd->command);
        }
    }
}

//
// kexInputKey::AddAction
//

void kexInputKey::AddAction(byte id, const char *name) {
    keyaction_t *keyaction;
    unsigned int hash;
    
    if(strlen(name) >= MAX_FILEPATH)
        kexlib::system->Error("Key_AddAction: \"%s\" is too long", name);

    if(!kexlib::commands->Verify(name)) {
        return;
    }

    keyaction = (keyaction_t*)Mem_Malloc(sizeof(keyaction_t), hb_static);
    keyaction->keyid = id;
    strcpy(keyaction->name, name);

    kexlib::commands->Add(keyaction->name, FCmd_KeyAction);

    hash = kexStr::Hash(keyaction->name);
    keyaction->next = keyactions[hash];
    keyactions[hash] = keyaction;
}

//
// kexInputKey::AddAction
//

void kexInputKey::AddAction(byte id, const kexStr &str) {
    AddAction(id, str.c_str());
}

//
// kexInputKey::WriteBindings
//

void kexInputKey::WriteBindings(FILE *file) {
    keycmd_t *keycmd;
    cmdlist_t *cmd;
    kexStr tmp;

    for(int i = 0; i < MAX_KEYS; i++) {
        keycmd = &keycmds[i];

        for(cmd = keycmd->cmds; cmd; cmd = cmd->next) {
            char buff[32];

            GetName(buff, i);
            tmp = buff;
            tmp.ToLower();

            fprintf(file, "bind %s \"%s\"\n", tmp.c_str(), cmd->command);
        }
    }
}

//
// kexInputKey::Init
//

void kexInputKey::Init(void) {
    for(int c = 0; c < MAX_KEYS; c++) {
        keycode[0][c] = c;
        keycode[1][c] = c;
        keydown[c] = false;
        keycmds[c].cmds = NULL;
    }

    keycode[1]['1'] = '!';
    keycode[1]['2'] = '@';
    keycode[1]['3'] = '#';
    keycode[1]['4'] = '$';
    keycode[1]['5'] = '%';
    keycode[1]['6'] = '^';
    keycode[1]['7'] = '&';
    keycode[1]['8'] = '*';
    keycode[1]['9'] = '(';
    keycode[1]['0'] = ')';
    keycode[1]['-'] = '_';
    keycode[1]['='] = '+';
    keycode[1]['['] = '{';
    keycode[1][']'] = '}';
    keycode[1]['\\'] = '|';
    keycode[1][';'] = ':';
    keycode[1]['\''] = '"';
    keycode[1][','] = '<';
    keycode[1]['.'] = '>';
    keycode[1]['/'] = '?';
    keycode[1]['`'] = '~';
    
    for(int c = 'a'; c <= 'z'; c++) {
        keycode[1][c] = toupper(c);
    }

    kexlib::system->Printf("Key System Initialized\n");
}
