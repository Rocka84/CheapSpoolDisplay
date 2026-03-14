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
3. ~~Edit / Create tags~~ Version 1 Done!
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
5. Support Snapmaker Proprietary tags
    * Complexity: High.
    * Official Snapmaker tags on spools use Mifare Classic 1K tags (ISO14443A)
    * Proprietary encrypted format with RSA signatures (hard to write, but readable)
    * Sector keys are derived per-tag via HKDF-SHA256 from a master salt key and tag UID
    * Would require:
        * ESP32 crypto library (SHA256/HKDF)
        * Master Salt Key (from firmware or user)
        * Mifare Classic authentication and multi-sector reading logic
        * Binary data parsing to map fields

Known issues
------------

* ~~When the display goes to sleep for the first time, it does wake on touch but turns off again after about a second. On subsequent behaves the same.~~ Fixed!
* ~~Ugly bars at edges of the logo on the idle screen~~ Fixed!
* ~~canceling the tool selection dialog does not go back to the info screen~~ Fixed!
* ~~on the device I have configured tools to be 4, but the tool selection dialog shows only 1 tool. I have tried to reset the device, but it does not help.~~ Fixed!


Fixes for Edit / Create tag
---------------------------

* ~~keyboard cant be closed once it's open~~ Fixed!
* ~~spool id should have a numeric keyboard~~ Fixed!
* ~~I don't like the color swatches. But I want a preview of the color entered in the hex field~~ Fixed!
* ~~Longshot: What would it take to get a colorwheel widget in lvgl? Is a downgrade to LVGL 8.x worth it? What would be the downsides?~~ Won't Do!
* ~~when entering a color, I don't want to enter the # sign. It should be cut off before editing and added back after editing. Put a # left of the input to make this more clear. if possible the keyboard should open in a hex mode~~ Fixed!
* ~~we need to validate the inputs before allowing to save. Invalid data should be red. At the moment I can put anything in e.g. the color field and it will be saved. This is not what I want.~~ Fixed!


Prepared prompts
----------------

