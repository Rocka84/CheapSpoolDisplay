# Supported RFID Standards

CheapSpoolDisplay is designed for maximum interoperability across the 3D printing ecosystem. It supports the three most prominent open-source standards for filament RFID tagging.

## Overview of Standards

| Standard         | Format | Features                          | Used By                 |
| ---------------- | ------ | --------------------------------- | ----------------------- |
| **OpenSpool**    | JSON   | High flexibility, human-readable. | Spoolman, Community     |
| **OpenPrintTag** | CBOR   | Region-based, binary efficient.   | VCOPT, Bambu Lab        |
| **OpenTag3D**    | Binary | Extremely compact, fast to parse. | OpenTag3D, Legacy tools |
| **Snapmaker**    | Binary | Proprietary (Read-only).          | Snapmaker U1, official   |

---

## Core Data Fields

These are the essential fields required for identifying the material and setting up the basic print parameters.

| Field (UI)       | Example Value | OpenSpool (JSON) | OpenPrintTag (CBOR) | OpenTag3D (Binary) | Snapmaker          | Spoolman API           |
| ---------------- | ------------- | ---------------- | ------------------- | ------------------ | ------------------ | ---------------------- |
| Brand            | Prusament     | `brand`          | Brand Name          | Brand Name         | Vendor Name        | `filament.vendor.name` |
| Material/Type    | PLA           | `type`           | Material Type       | Material           | Main Type          | `filament.material`    |
| Filament         | Galaxy Black  | `filament_name`  | Material Name       | Product Name       | N/A                | `filament.name`        |
| Color (Hex)      | #FF0000       | `color_hex`      | Primary Color       | Color RGB          | RGB Color          | `filament.color_hex`   |
| Nozzle Temp      | 215           | `min_temp`       | Print Temperature   | Max Nozzle Temp    | Hotend Temp        | `settings_extruder`    |
| Bed Temp         | 60            | `bed_min`        | Bed Temperature     | Max Bed Temp       | Bed Temp           | `settings_bed`         |
| Total Weight     | 1000          | `total_weight`   | Nominal Net Weight  | Total Weight       | Total Weight       | `initial_weight`       |
| Remaining Weight | 750           | N/A              | N/A                 | N/A                | N/A                | `remaining_weight`     |
| Spool ID         | 123           | `spool_id`       | Instance UUID*      | ID via URL         | N/A                | `spool.id`             |

---

## Extended & Inventory Data

These fields provide technical material properties, manufacturing tracking, and inventory management data.

| Field (UI)         | Example Value  | OpenSpool (JSON) | OpenPrintTag (CBOR) | OpenTag3D (Binary) | Snapmaker          | Spoolman API        |
| ------------------ | -------------- | ---------------- | ------------------- | ------------------ | ------------------ | ------------------- |
| Diameter           | 1.75           | `diameter`       | Filament Diameter   | Diameter           | Diameter           | `filament.diameter` |
| Subtype            | Pro            | `subtype`        | Abbreviation        | Modifier           | Sub Type           | N/A (Extra Field)   |
| Lot Number         | 2024-A1        | `lot_nr`         | Lot Number          | Batch ID           | Tag UID            | `spool.lot_nr`      |
| Actual Net Weight  | 1012           | `actual_weight`  | Actual Net Weight   | Measured Weight    | N/A                | N/A (Extra Field)   |
| Empty Spool Weight | 250            | `empty_weight`   | Empty Spool Weight  | Empty Spool Weight | N/A                | `spool_weight`      |
| Density            | 1.24           | `density`        | Material Density    | Density            | N/A                | `filament.density`  |
| Drying Temp        | 50             | `dry_temp`       | Drying Temperature  | Max Dry Temp       | Drying Temp        | N/A (Extra Field)   |
| Drying Time        | 240            | `dry_time`       | Drying Time         | Dry Time           | Drying Time        | N/A (Extra Field)   |
| TD (Opacity)       | 6.6            | `td`             | Transm. Distance    | Opacity            | N/A                | N/A (Extra Field)   |
| Shore Hardness     | 95A            | `shore`          | Shore Hardness      | N/A                | N/A                | N/A (Extra Field)   |
| Tags               | wood, glitter  | `tags`           | Material Tags       | N/A                | N/A                | N/A (Extra Field)   |
| Storage Location   | Shelf B3       | `location`       | Storage Location    | N/A                | N/A                | `spool.location`    |
| Price              | 19.99          | N/A              | N/A                 | N/A                | N/A                | `spool.price`       |
| Notes              | Dry before use | N/A              | N/A                 | N/A                | N/A                | `spool.comment`     |
| First Used         | 2024-01-01     | N/A              | N/A                 | N/A                | N/A                | `spool.first_used`  |
| Last Used          | 2024-05-01     | N/A              | N/A                 | N/A                | N/A                | `spool.last_used`   |

---

## Technical Details

### OpenSpool (JSON)
*   **Storage**: NDEF MIME Record `application/json`
*   **Encoding**: Plain text UTF-8 JSON.
*   **Best For**: Direct integration with Spoolman and human-readable tag verification.

### OpenPrintTag (CBOR)
*   **Storage**: NDEF MIME Record `application/vnd.openprinttag`
*   **Encoding**: Concise Binary Object Representation (CBOR) using a region-based offset table.
*   **Structure**: 
    1.  **Meta Section**: Map containing offsets for Main and Auxiliary data.
    2.  **Main Section**: Static filament properties (Brand, Color, etc.).
    3.  **Auxiliary Section**: Dynamic data (Remaining weight/consumed weight).
*   **Spoolman ID Handling**: To maintain compatibility with the spec's 16-byte `Instance UUID` field, integer Spoolman IDs are encoded as a specialized UUID with the prefix `SPMN` (e.g., ID `123` becomes `53504d4e-0000-0000-0000-00000000007b`).

### OpenTag3D (Binary)
*   **Storage**: NDEF MIME Record `application/opentag3d`
*   **Encoding**: Fixed-offset binary structure.
*   **Note**: Temperatures are stored as Celsius divided by 5 to fit in a single byte.

### Snapmaker (Proprietary)
*   **Storage**: Mifare Classic 1K Sectors 0-9.
*   **Authentication**: Proprietary HKDF-derived keys.
*   **Support Level**: Read-only.
*   **Spoolman Linking**: Since official tags cannot be modified, they can be linked to Spoolman by setting the spool's **Lot Number** in Spoolman to the tag's 8-character hex UID (e.g. `A1B2C3D4`). 
    - The search is performed as an **exact match** (using quotes in the API).
    - The device will automatically find and sync with the corresponding spool on scan.
*   **Support**: **Read-only**. Official tags are signed with an RSA-2048 private key owned by Snapmaker.
*   **Fields**: Supports Brand, Material Type, Subtype, Color, Diameter, Weight, and Temperature settings.
