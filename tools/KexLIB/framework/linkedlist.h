// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2013 Samuel Villarreal
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

#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

template<class type>
class kexLinklist {
public:
                    kexLinklist();
                    ~kexLinklist();
                    
    void            Add(kexLinklist &link);
    void            AddBefore(kexLinklist &link);
    void            Remove(void);
    void            Reset(void);
    void            Clear(void);
    int             GetCount(void) const;
    void            SetData(type *src);
    type            *GetData(void) const;
    type            *Next(void) const;
    type            *Prev(void) const;
    bool            Contains(type *check);
    
private:
    kexLinklist     *next;
    kexLinklist     *prev;
    type            *data;
};

//
// kexLinklist::kexLinklist
//
template<class type>
kexLinklist<type>::kexLinklist() {
    Reset();
}

//
// kexLinklist::~kexLinklist
//
template<class type>
kexLinklist<type>::~kexLinklist() {
}

//
// kexLinkList::Reset
//
template<class type>
void kexLinklist<type>::Reset(void) {
    data    = NULL;
    next    = this;
    prev    = this;
}

//
// kexLinklist::Add
//
template<class type>
void kexLinklist<type>::Add(kexLinklist &link) {
    prev        = &link;
    next        = link.next;
    link.next   = this;
    next->prev  = this;
}

//
// kexLinklist::AddBefore
//
template<class type>
void kexLinklist<type>::AddBefore(kexLinklist &link) {
    next        = &link;
    prev        = link.prev;
    link.prev   = this;
    prev->next  = this;
}

//
// kexLinklist:Remove
//
template<class type>
void kexLinklist<type>::Remove(void) {
    next->prev = prev;
    prev->next = next;

    next = prev = this;
}

//
// kexLinklist:Clear
//
template<class type>
void kexLinklist<type>::Clear(void) {
    while(next != this) {
        if(next == NULL) {
            break;
        }
        next->Remove();
    }

    next = prev = this;
}

//
// kexLinklist::GetCount
//
template<class type>
int kexLinklist<type>::GetCount(void) const {
    int count = 0;
    for(kexLinklist<type> *link = next; link != this; link = link->next) {
        count++;
    }
    
    return count;
}

//
// kexLinklist::Next
//
template<class type>
type *kexLinklist<type>::Next(void) const {
    return next->data;
}

//
// kexLinklist::Prev
//
template<class type>
type *kexLinklist<type>::Prev(void) const {
    return prev->data;
}

//
// kexLinklist::SetData
//
template<class type>
void kexLinklist<type>::SetData(type *src) {
    data = src;
}

//
// kexLinklist::GetData
//
template<class type>
type *kexLinklist<type>::GetData(void) const {
    return data;
}

//
// kexLinklist::Contains
//
template<class type>
bool kexLinklist<type>::Contains(type *check) {
    for(kexLinklist<type> *link = next; link != this; link = link->next) {
        if(data == check) {
            return true;
        }
    }
    return false;
}

#endif
