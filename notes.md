Ideas
-----

1. ~~Enrich the info on the display with information from Spoolman server~~ Done!
   * optional, only when spoolman url and wifi is configured
   * wifi must be enabled if spoolman url is configured (atm it's only activated when a webhook is configured)
   * only when spool_id is set
   * show remaining and total weight
   * show filament name
2. ~~Create 3D printable case for the device in OpenSCAD~~ Postponed!
    * RFID reader at the back of the display
    * must encase both screen and reader in a minimal way
    * reader must be accessible for NFC tags
    * touchscreen must be accessible
    * must be printable with no or minimal supports
    * unavoidable supports must be builtin and easy to remove
    * usbc must be accessible, all other connectors must be covered
    * both boards must be mountable
3. Edit / Create tags
    * when a tag is scanned, show an edit button
    * when edit is pressed, show an edit screen
    * on edit screen, show the current values
    * allow editing of all values
    * allow saving the tag
    * allow cancelling the edit
    * on save, ask user to present tag and write to it
    * on cancel, go back to info screen
    * on idle screen, show a button to create a new tag
    * for selects use same values as PrintTag-web
        * source: /home/dilli/src/PrintTag-Web
        * additional brands: "Das Filament" and "RecyclingFabrik"
    * open question: How to handle Custom Brand names, is there a onscreen keyboard widget or something like that in lvgl?
4. ~~Snapmaker U1 Extended Firmware support~~ Done!
    * feature was merged to extended firmware recently: https://github.com/paxx12/SnapmakerU1-Extended-Firmware/pull/314
    * Example implementation: https://github.com/wasikuss/snapmaker-u1-remote-rfid-reader
    * alternative to webhook
    * sends information as GCODE to the printer similar to example implementation
    * address of U1 must be configurable
    * when configured, ignore webhook

Known issues
------------

* ~~When the display goes to sleep for the first time, it does wake on touch but turns off again after about a second. On subsequent behaves the same.~~ Fixed!
* ~~Ugly bars at edges of the logo on the idle screen~~ Fixed!
* ~~canceling the tool selection dialog does not go back to the info screen~~ Fixed!
* ~~on the device I have configured tools to be 4, but the tool selection dialog shows only 1 tool. I have tried to reset the device, but it does not help.~~ Fixed!