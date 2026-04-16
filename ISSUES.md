### Problem 1: `analytics.h/cpp` lives in `bookkeeping/` but is pure UI code

**Files:** `bookkeeping/analytics.h`, `bookkeeping/analytics.cpp`

`WalletsAnalytics` and `CategoriesAnalytics` are entirely UI classes — they own `QChart*`, `QPieSeries*`, `QChartView*`, `QComboBox*`, `QPushButton*`, `QDateTimeEdit*`, and they call `initUi(Ui::MainWindow*)` to grab widget pointers. They also include `gui/widgets/widgets.h`. Yet they live in `bookkeeping/`, a directory that is supposed to be the core business logic layer.

**Fix:** Move `analytics.h/cpp` to `gui/forms/analytics/` (or a new `gui/analytics/`). The chart/filter computation logic that is truly reusable (e.g., the wallet filtering + grouping in `WalletsAnalytics::updateAnalytics`, the date-bucketed spending scan in `CategoriesAnalytics::updateAnalytics`) could be extracted into a pure data function in `bookkeeping/` that returns a `std::map<QString, Money>` or similar, and the UI classes would just call that function and feed results into the chart.

---

### Problem 2: `models.cpp` includes and creates GUI widgets

**File:** `bookkeeping/models.cpp:2` — `#include "gui/widgets/widgets.h"`

The `ModelsDelegate` class (defined in `bookkeeping/models.h:408-424`, implemented in `bookkeeping/models.cpp:1574-1815`) creates concrete GUI widgets: `QCheckBox`, `QComboBox`, `QDateEdit`, `RelaxedDoubleSpinBox`, `CategoryNodeButton`, `WalletNodeButton`. It also casts to specific model types (`LogModel`, `PlansModel`, `TasksModel`) to decide which editor to create. This creates a circular dependency: `bookkeeping/models.cpp` depends on `gui/widgets/`, while `gui/` depends on `bookkeeping/models.h`.

**Fix:** Move `ModelsDelegate` out of `bookkeeping/models.h/cpp` into its own file under `gui/`, e.g., `gui/modelsdelegate.h/cpp`. The delegate is fundamentally a UI concern — it paints cells and creates editor widgets. The pure data models (`CategoriesModel`, `LogModel`, etc.) can stay in `bookkeeping/`.

---

### Problem 3: Receipt HTML parsing in `MainWindow::on_actionImportReceipt_triggered`

**File:** `gui/forms/mainwindow.cpp:1163-1262`

The Serbian receipt import flow performs HTTP fetching (`QNetworkAccessManager`) and HTML content parsing (extracting text between `<pre style=` and `</pre>`, then finding `===` delimiters and `"Назив"` markers) directly inside the UI slot. The `MainWindow` should not be responsible for network I/O or HTML scraping.

**Fix:** The network fetching + HTML extraction should live in a dedicated service/helper (e.g., `SerbReceiptFetcher` or be part of `Data`). The UI slot should only:
1. Show dialogs to get user input (URL, wallet selection)
2. Call the service to fetch + parse
3. Feed results back to `Data::importSerbReceiptFile`

Note: `Data::importSerbReceiptFile` already exists and handles the actual receipt text parsing — the problem is just the HTML-to-text extraction and HTTP fetch that lives in the UI.

---

### Problem 4: `showUnanchoredSum` computes business data

**File:** `gui/forms/mainwindow.cpp:910-920`

```cpp
void MainWindow::showUnanchoredSum()
{
    Money sum;
    for(int i = 0; i<m_models.logModel.m_data.unanchored; ++i) {
        sum += m_models.logModel.m_data.log[i].amount;
    }
    ui->unanchoredSumLabel->setText(formatMoney(sum));
    ...
}
```

The UI is iterating over raw transaction data and computing a sum. This is a business logic calculation.

**Fix:** Add a method to `LogData` like `Money unanchoredSum() const` that performs this computation, then the UI just calls `formatMoney(m_models.logModel.m_data.unanchoredSum())`.

---

### Problem 5: `TreemapModel::showCategoryStatement` creates a UI window

**File:** `gui/forms/analytics/categoriesstaticchart.cpp:312-332`

`TreemapModel` is a `QObject`-based data model, but `showCategoryStatement()` creates a `QTableView`, sets window flags, configures columns, and shows it full-screen. This is purely UI logic embedded in a model class.

**Fix:** Emit a signal from `TreemapModel` (e.g., `void categoryStatementRequested(const Node<Category>* category)`) and handle the window creation in the UI layer (e.g., in `MainWindow::postLoadSetup` where the model is connected). Alternatively, move this into a factory function in a `gui/` file.

---

### Problem 6: `TreemapModel` memory leak + ownership issue

**File:** `gui/forms/mainwindow.cpp:317`

```cpp
TreemapModel* p = new TreemapModel;
```

This is allocated with `new` but never assigned as a child of any QObject with proper ownership, and there's no explicit `delete`. Since `TreemapModel` is a `QObject`, it *could* be parented, but it isn't here. Also, lambdas capture `p` as raw pointer — if `MainWindow` is destroyed, the connections would dangle.

**Fix:** Create with parent: `new TreemapModel(this)`, or make it a member of `MainWindow`.

---

### Problem 7: `BriefModel::data` embeds visual formatting logic

**File:** `bookkeeping/models.cpp:1444-1552`

`BriefModel::data()` returns `QFont("Segoe UI", 8/9)` with bold styling, `QColor` values, and text alignment directly. This mixes visual presentation with the data model. Other models in this file also return colors (via the `colors::` namespace), but `BriefModel` is the most egregious — it hardcodes font family and size.

**Fix (low priority):** Move font/color decisions into the delegate or use a stylesheet. At minimum, the font names and sizes should not be hardcoded in the model — they should come from the delegate or a theme configuration.

---

### Problem 8: `LogModel::removeRows` performs business logic

**File:** `bookkeeping/models.cpp:854-868`

```cpp
m_data.log.erase(...);
m_data.unanchored -= 1;
```

The model directly mutates `LogData` internals (erasing from the deque, decrementing the `unanchored` counter). Compare with `LogModel::insertRows` which properly delegates to `m_data.insertRows()`. The remove operation should similarly be delegated to `LogData`.

**Fix:** Add `LogData::removeRow(int position)` and call it from `LogModel::removeRows`, mirroring the pattern already used for `insertRows`.

---

### Summary — prioritized

| # | Severity | Issue | Where | Fix |
|---|----------|-------|-------|-----|
| 1 | **High** | Analytics classes are UI but live in `bookkeeping/` | `bookkeeping/analytics.*` | Move to `gui/`, extract pure computation to core |
| 2 | **High** | `ModelsDelegate` creates widgets from `bookkeeping/` | `bookkeeping/models.*` | Move delegate to `gui/` |
| 3 | **Medium** | HTTP fetch + HTML parsing in UI slot | `mainwindow.cpp:1163-1262` | Extract to service class |
| 4 | **Low** | Unanchored sum computed in UI | `mainwindow.cpp:910-920` | Add `LogData::unanchoredSum()` |
| 5 | **Medium** | Model class creates UI window | `categoriesstaticchart.cpp:312-332` | Use signal, handle in UI |
| 6 | **Low** | `TreemapModel` memory leak | `mainwindow.cpp:317` | Parent to `this` or make member |
| 7 | **Low** | Hardcoded fonts/colors in model | `models.cpp:1527-1539` | Move to delegate/stylesheet |
| 8 | **Low** | `LogModel::removeRows` bypasses `LogData` | `models.cpp:854-868` | Add `LogData::removeRow()` |
