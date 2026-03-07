Ideas
-----

1. Enrich the info on the tag with information form Spoolman server
   * optional, only when spoolman url and wifi is configured
   * wifi must be enabled if spoolman url is configured (atm it's only activated when a webhook is configured)
   * only when spool_id is set
   * show remaining and total weight
   * show filament name
2. Create 3D printable case for the device in OpenSCAD
    * RFID reader at the back of the display
    * must encase both screen and reader in a minimal way
    * reader must be accessible for NFC tags
    * touchscreen must be accessible
    * must be printable with no or minimal supports
    * unavoidable supports must be builtin and easy to remove
    * usbc must be accessible, all other connectors must be covered
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