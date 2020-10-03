#pragma once

class Viewport;

/// Editing state -> this could be embedded in the manipulator ???
struct ViewportEditor {
    virtual ~ViewportEditor() {};
    virtual void OnBeginEdition(Viewport &) {}; // Enter State
    virtual void OnEndEdition(Viewport &) {};  // Exit State
    // BeginModification(); // Store the first modification
    // EndModification();   // Store the last commands ?
    virtual ViewportEditor *  OnUpdate(Viewport &) = 0; // OnUpdate()

    virtual bool IsMouseOver(const Viewport &) { return false; };

    /// Draw the translate manipulator as seen in the viewport
    virtual void OnDrawFrame(const Viewport &) {};

};