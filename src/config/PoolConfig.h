// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Central compile-time limits for input resource pools.
// Adjust these values to match the maximum supported hardware configuration.

#ifndef MAX_BUTTON_PIN_GROUPS
#define MAX_BUTTON_PIN_GROUPS 32
#endif

#ifndef MAX_LOGICAL_PER_PIN
#define MAX_LOGICAL_PER_PIN 4
#endif

#ifndef MAX_SHIFTREG_GROUPS
#define MAX_SHIFTREG_GROUPS 32
#endif

#ifndef MAX_LOGICAL_PER_SHIFT_BIT
#define MAX_LOGICAL_PER_SHIFT_BIT 4
#endif

// Matrix limits
#ifndef MAX_MATRIX_ROWS
#define MAX_MATRIX_ROWS 8
#endif

#ifndef MAX_MATRIX_COLS
#define MAX_MATRIX_COLS 8
#endif

#ifndef MAX_LOGICAL_PER_MATRIX_POS
#define MAX_LOGICAL_PER_MATRIX_POS 4
#endif

// Encoder limits
#ifndef MAX_ENCODERS
#define MAX_ENCODERS 8
#endif
// (Duplicate matrix/encoder limit block removed)
