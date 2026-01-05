# MetadataPanel Refactoring Summary

## Requested Changes

1. **Tab Order**: Metadata tab should be first and active by default, Semantic Keys last
2. **Dynamic Fields**: Replace hardcoded fields with dynamic loading based on actual metadata
3. **Collapsible Sections**: Organize metadata into collapsible groups (EXIF, IPTC, XMP, File, Technical, Custom)
4. **Add New Fields**: Allow users to insert new custom metadata fields
5. **Update Tests**: Reflect new functionality in tests

## Implementation Approach

Due to the complexity of a complete rewrite, I recommend an incremental approach:

### Phase 1: Add Tab Structure (COMPLETE)
- Header updated with QTabWidget and new class structures
- Metadata tab first, Semantic Keys tab last  
- Tab widget active by default on Metadata

### Phase 2: Dynamic Metadata Fields (IN PROGRESS)
The current implementation has hardcoded fields. We need to:

1. **Read ALL metadata** using ExifTool with `-G` flag to get group prefixes
2. **Categorize by group**: EXIF:, IPTC:, XMP:, File:
3. **Create CollapsibleGroupBox** widgets for each category
4. **Populate dynamically** with MetadataFieldWidget instances

### Phase 3: Custom Fields
- Add "+ Add Field" button (visible in edit mode)
- Dialog to enter field name and value
- Save to XMP-photoguru namespace
- Display in "Custom Fields" collapsible section

### Phase 4: Update MainWindow
Currently panels are tabbed as: Metadata, SKP, Analysis
Need to change order in `createDockPanels()` to: Metadata, Analysis, SKP (Semantic Keys)

## Files Modified

- `src/ui/MetadataPanel.h` - ✅ DONE - Added new classes and members
- `src/ui/MetadataPanel.cpp` - ⚠️ IN PROGRESS - Need to complete implementation
- `src/ui/MainWindow.cpp` - ⏳ TODO - Reorder dock panels  
- `tests/test_metadata_panel.cpp` - ⏳ TODO - Update tests

## Key Classes Added

### CollapsibleGroupBox
```cpp
class CollapsibleGroupBox : public QWidget {
    Q_OBJECT
public:
    explicit CollapsibleGroupBox(const QString& title, QWidget* parent = nullptr);
    void setContentLayout(QLayout* layout);
    bool isExpanded() const;
private slots:
    void toggleExpanded();
private:
    QPushButton* m_toggleButton;
    QWidget* m_contentWidget;
    bool m_expanded;
};
```

### MetadataFieldWidget  
```cpp
class MetadataFieldWidget : public QWidget {
    Q_OBJECT
public:
    MetadataFieldWidget(const QString& key, const QString& value, bool editable, QWidget* parent);
    QString key() const;
    QString value() const;
    void setValue(const QString& value);
    void setEditable(bool editable);
    bool isModified() const;
signals:
    void valueChanged(const QString& key, const QString& value);
    void removeRequested(const QString& key);
};
```

## Next Steps

1. Complete MetadataPanel.cpp implementation with all new methods
2. Test compilation
3. Update MainWindow panel order
4. Update tests
5. Test with actual images

## Notes

- ExifTool daemon should be used for performance
- Custom fields saved to XMP-photoguru: namespace
- Maintain backward compatibility with existing metadata
- Quick Edit section remains for commonly used fields (rating, title, description, keywords, category, location)
