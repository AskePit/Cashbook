

### Problem 7: `BriefModel::data` embeds visual formatting logic

**File:** `bookkeeping/models.cpp:1444-1552`

`BriefModel::data()` returns `QFont("Segoe UI", 8/9)` with bold styling, `QColor` values, and text alignment directly. This mixes visual presentation with the data model. Other models in this file also return colors (via the `colors::` namespace), but `BriefModel` is the most egregious — it hardcodes font family and size.

**Fix (low priority):** Move font/color decisions into the delegate or use a stylesheet. At minimum, the font names and sizes should not be hardcoded in the model — they should come from the delegate or a theme configuration.

---

### Summary — prioritized

| # | Severity | Issue | Where | Fix |
|---|----------|-------|-------|-----|
| 7 | **Low** | Hardcoded fonts/colors in model | `models.cpp:1527-1539` | Move to delegate/stylesheet |