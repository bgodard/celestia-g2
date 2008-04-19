// qtbookmark.cpp
//
// Copyright (C) 2003-2008, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// Celestia bookmark structure.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "qtbookmark.h"
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QMessageBox>
#include <QHeaderView>
#include <QMimeData>
#include <QStringList>
#include <QMenu>
#include "xbel.h"


BookmarkItem::BookmarkItem(Type type, BookmarkItem* parent) :
    m_type(type),
    m_parent(parent),
    m_position(0)
{

}


BookmarkItem::Type
BookmarkItem::type() const
{
    return m_type;
}


BookmarkItem* 
BookmarkItem::parent() const
{
    return m_parent;
}


// private
void
BookmarkItem::setParent(BookmarkItem* parent)
{
    m_parent = parent;
}


QString
BookmarkItem::title() const
{
    return m_title;
}


void 
BookmarkItem::setTitle(const QString& title)
{
    m_title = title;
}


QString
BookmarkItem::url() const
{
    return m_url;
}


bool
BookmarkItem::folded() const
{
    return m_folded;
}


void
BookmarkItem::setFolded(bool folded)
{
    m_folded = folded;
}


QString
BookmarkItem::description() const
{
    return m_description;
}


void 
BookmarkItem::setDescription(const QString& description)
{
    m_description = description;

}

int
BookmarkItem::childCount() const
{
    return m_children.size();
}


BookmarkItem*
BookmarkItem::child(int index) const
{
    if (index < 0 || index >= m_children.size())
        return NULL;
    else
        return m_children[index];
}


bool
BookmarkItem::isRoot() const
{
    return m_parent == NULL;
}


void
BookmarkItem::setUrl(const QString& url)
{
    m_url = url;
}


int
BookmarkItem::position() const
{
    if (m_parent == NULL)
        return 0;
    else
        return m_parent->childPosition(this);
}


int
BookmarkItem::childPosition(const BookmarkItem* child) const
{
    int n = 0;
    foreach (BookmarkItem* c, m_children)
    {
        if (c == child)
            return n;
        n++;
    }

    // no match found
    return -1;
}


void
BookmarkItem::insert(BookmarkItem* child, int beforeIndex)
{
    Q_ASSERT(type() == Folder);
    Q_ASSERT(beforeIndex <= m_children.size());
    Q_ASSERT(child->parent() == this);

    m_children.insert(m_children.begin() + beforeIndex, child);
}


void
BookmarkItem::append(BookmarkItem* child)
{
    insert(child, m_children.size());
}


void
BookmarkItem::removeChildren(int index, int count)
{
    Q_ASSERT(index + count <= m_children.size());
    for (int i = 0; i < count; i++)
    {
        m_children[index + i]->setParent(NULL);
        //m_children[index + i]->setPosition(0);
    }

    m_children.erase(m_children.begin() + index, m_children.begin() + index + count);
}


BookmarkItem*
BookmarkItem::clone(BookmarkItem* withParent) const
{
    BookmarkItem* newItem = new BookmarkItem(m_type, withParent);
    newItem->m_title = m_title;
    newItem->m_url = m_url;
    newItem->m_description = m_description;
    newItem->m_folded = m_folded;
    foreach (BookmarkItem* child, m_children)
    {
        newItem->m_children += child->clone(newItem);
    }

    return newItem;
}


/***** BookmarkTreeModel *****/

BookmarkTreeModel::BookmarkTreeModel() :
    m_root(NULL)
{
}


BookmarkTreeModel::~BookmarkTreeModel()
{
    delete m_root;
}
    

const BookmarkItem*
BookmarkTreeModel::getItem(const QModelIndex& index) const
{
    return static_cast<const BookmarkItem*>(index.internalPointer());
}


BookmarkItem* 
BookmarkTreeModel::getItem(const QModelIndex& index)
{
    return static_cast<BookmarkItem*>(index.internalPointer());
}


QModelIndex
BookmarkTreeModel::itemIndex(BookmarkItem* item)
{
    if (item->isRoot())
        return QModelIndex();
    else
        return createIndex(item->position(), 0, item);
}


QModelIndex 
BookmarkTreeModel::index(int row, int /* column */, const QModelIndex& parent) const
{
    const BookmarkItem* parentFolder = NULL;
    
    if (!parent.isValid())
        parentFolder = m_root;
    else
        parentFolder = getItem(parent);
    
    Q_ASSERT(parentFolder->type() == BookmarkItem::Folder);
    Q_ASSERT(row < (int) parentFolder->childCount());

    return createIndex(row, 0, const_cast<void*>(reinterpret_cast<const void*>(parentFolder->child(row))));
}


QModelIndex
BookmarkTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();
    
    const BookmarkItem* item = getItem(index);
    BookmarkItem* parentFolder = item->parent();
    if (parentFolder == m_root)
        return QModelIndex();
    else
        return createIndex(parentFolder->position(), 0, reinterpret_cast<void*>(parentFolder));
}


int 
BookmarkTreeModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid())
        return m_root->childCount();
    else
        return getItem(parent)->childCount();
}


int
BookmarkTreeModel::columnCount(const QModelIndex& /* parent */) const
{
    return 1;
}


QVariant 
BookmarkTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();
    
    const BookmarkItem* item = getItem(index);

    switch (role)
    {
    case UrlRole:
        if (item->type() == BookmarkItem::Bookmark)
            return item->url();
        else
            return QVariant();

    case TypeRole:
        return item->type();

    case Qt::DisplayRole:
        if (item->type() == BookmarkItem::Separator)
            return QString(40, QChar('-'));
        else
            return item->title();

    case Qt::DecorationRole:
        if (index.column() == 0 &&
            item->type() == BookmarkItem::Folder)
        {
            return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
        }
        else
        {
            return QVariant();
        }

    default:
        return QVariant();
    }
}


bool
BookmarkTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (index.isValid() && role == Qt::EditRole)
    {
        BookmarkItem* item = getItem(index);
        item->setTitle(value.toString());
        emit dataChanged(index, index);
        return true;
    }
    else
    {
        return false;
    }
}


QVariant
BookmarkTreeModel::headerData(int /* section */, Qt::Orientation /* orientation */, int /* role */) const
{
    return QString();
}


Qt::ItemFlags
BookmarkTreeModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    if (index.isValid())
    {
        flags |= Qt::ItemIsDragEnabled;
        const BookmarkItem* item = getItem(index);
        if (item->type() == BookmarkItem::Folder)
            flags |= Qt::ItemIsDropEnabled;
        else if (item->type() == BookmarkItem::Separator)
            flags &= ~(Qt::ItemIsEnabled | Qt::ItemIsEditable);
    }
    else
    {
        flags |= Qt::ItemIsDropEnabled;
    }

    return flags;
}


Qt::DropActions
BookmarkTreeModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}


bool 
BookmarkTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (action == Qt::IgnoreAction)
        return true;

    if (column > 0)
        return false;

    if (!data->hasFormat("application/celestia.text.list"))
        return false;

    QByteArray encodedData = data->data("application/celestia.text.list");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    BookmarkItem* item = NULL;

    // Read the pointer (ugh) from the encoded mime data. Bail out now
    // if the data was incomplete for some reason.
    if (stream.readRawData(reinterpret_cast<char*>(&item), sizeof(item)) != sizeof(item))
        return false;

    BookmarkItem* parentFolder = NULL;       
    if (!parent.isValid())
        parentFolder = m_root;
    else
        parentFolder = getItem(parent);

    // It should not be possible for this method to be called on something other than a group.
    Q_ASSERT(parentFolder->type() == BookmarkItem::Folder);

    // Row of -1 indicates that we're free to place the moved item anywhere within the group.
    // The convention is to append the item.
    if (row == -1)
        row = parentFolder->childCount();

    // Drag and drop doesn't allow for atomic reparenting; first the dragged item
    // is inserted in the new position, then the old item is removed. Bad things
    // happen when the item appears in two separate places, so we'll insert a copy
    // of the old data into the new position.
    BookmarkItem* clone = item->clone(parentFolder);

    beginInsertRows(parent, row, row);
    parentFolder->insert(clone, row);
    endInsertRows();

    return true;
}


bool
BookmarkTreeModel::removeRows(int row, int count, const QModelIndex& parent)
{
    BookmarkItem* parentFolder = NULL;       
    if (!parent.isValid())
        parentFolder = m_root;
    else
        parentFolder = getItem(parent);

    beginRemoveRows(parent, row, row + count - 1);
    parentFolder->removeChildren(row, count);
    endRemoveRows();

    return true;
}


QStringList
BookmarkTreeModel::mimeTypes() const
{
    QStringList types;
    types << "application/celestia.text.list";
    return types;
}


QMimeData*
BookmarkTreeModel::mimeData(const QModelIndexList& indexes) const
{
    if (indexes.size() != 1)
        return NULL;

    QMimeData* mimeData = new QMimeData();
    QByteArray encodedData;

    const BookmarkItem* item = getItem(indexes.at(0));
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    stream.writeRawData(reinterpret_cast<const char*>(&item), sizeof(item));
    mimeData->setData("application/celestia.text.list", encodedData);

    return mimeData;
}


void
BookmarkTreeModel::addItem(BookmarkItem* item, int position)
{
    // Can't insert the root item
    Q_ASSERT(!item->isRoot());

    BookmarkItem* parentItem = item->parent();
    QModelIndex parentIndex = itemIndex(parentItem);
    this->beginInsertRows(parentIndex, position, position);
    parentItem->insert(item, position);
    this->endInsertRows();
}


void
BookmarkTreeModel::removeItem(BookmarkItem* item)
{
    Q_ASSERT(!item->isRoot());

    int position = item->position();

    BookmarkItem* parentItem = item->parent();
    QModelIndex parentIndex = itemIndex(parentItem);
    this->beginRemoveRows(parentIndex, position, position);
    parentItem->removeChildren(position, 1);
    this->endInsertRows();
}

//    void changeItemTitle(BookmarkItem* item, const QString& newTitle);


BookmarkManager::BookmarkManager(QObject* parent) :
    QObject(parent),
    m_root(NULL),
    m_model(NULL)
{
    m_model = new BookmarkTreeModel();
}


BookmarkTreeModel*
BookmarkManager::model() const
{
    return m_model;
}


void
BookmarkManager::initializeBookmarks()
{
    m_root = new BookmarkItem(BookmarkItem::Folder, NULL);
    m_root->setTitle("root");
    
    BookmarkItem* menuBookmarks = new BookmarkItem(BookmarkItem::Folder, m_root);
    menuBookmarks->setTitle(tr("Bookmarks Menu"));
    m_root->append(menuBookmarks);

    BookmarkItem* toolbarBookmarks = new BookmarkItem(BookmarkItem::Folder, m_root);
    toolbarBookmarks->setTitle(tr("Bookmarks Toolbar"));
    m_root->append(toolbarBookmarks);

    m_model->m_root = m_root;
}


bool
BookmarkManager::loadBookmarks(QIODevice* device)
{
    XbelReader reader(device);
    m_root = reader.read();
    if (m_root == NULL)
        QMessageBox::warning(NULL, tr("Error reading bookmarks file"), reader.errorString());

    m_model->m_root = m_root;

    return m_root != NULL;
}


bool
BookmarkManager::saveBookmarks(QIODevice* device)
{
    XbelWriter writer(device);
    return writer.write(m_model->m_root);
}


void
BookmarkManager::populateBookmarkMenu(QMenu* menu)
{
    appendBookmarkMenuItems(menu, m_root);
}


QMenu*
BookmarkManager::createBookmarkMenu(QMenu* parent, const BookmarkItem* item)
{
    QMenu* menu = new QMenu(item->title(), parent);
    appendBookmarkMenuItems(menu, item);
    return menu;
}


void
BookmarkManager::appendBookmarkMenuItems(QMenu* menu, const BookmarkItem* item)
{
    for (int i = 0; i < item->childCount(); i++)
    {
        BookmarkItem* child = item->child(i);
        switch (child->type())
        {
        case BookmarkItem::Folder:
            if (child->childCount() > 0)
            {
                QMenu* submenu = createBookmarkMenu(menu, child);
                menu->addMenu(submenu);
            }
            break;

        case BookmarkItem::Bookmark:
            {
                QAction* action = new QAction(child->title(), menu);
                action->setData(child->url());
                connect(action, SIGNAL(triggered()), this, SLOT(bookmarkMenuItemTriggered()));
                menu->addAction(action);
            }
            break;

        case BookmarkItem::Separator:
            menu->addSeparator();
            break;
        }
    }   
}


void
BookmarkManager::bookmarkMenuItemTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action != NULL)
    {
        emit bookmarkTriggered(action->data().toString());
    }
}


// Proxy model which filters out all items in the bookmark list that
// aren't folders.
class OnlyFoldersProxyModel : public QSortFilterProxyModel
{
public:
    OnlyFoldersProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
    }

    bool filterAcceptsRow(int row, const QModelIndex& parent) const
    {
        QModelIndex index = sourceModel()->index(row, 0, parent);
        BookmarkItem::Type type = static_cast<BookmarkItem::Type>(sourceModel()->data(index, BookmarkTreeModel::TypeRole).toInt());
        return type == BookmarkItem::Folder;
    }
};


AddBookmarkDialog::AddBookmarkDialog(BookmarkManager* manager,
                                     QString defaultTitle,
                                     QString url) :
    m_manager(manager),
    m_filterModel(NULL),
    m_url(url)
{
    setupUi(this);
    bookmarkNameEdit->setText(defaultTitle);

    BookmarkTreeModel* model = manager->model();

    // User is only allowed to create a new bookmark in a folder
    // Filter out non-folders in the "create in" combo box
    QTreeView* view = new QTreeView(this); 
    m_filterModel = new OnlyFoldersProxyModel(this);
    m_filterModel->setSourceModel(model);

    view->setModel(m_filterModel);
    view->expandAll();
    view->header()->hide();
    view->setRootIsDecorated(false);
    view->setItemsExpandable(false);

    createInCombo->setModel(m_filterModel);
    // Initialize to first index
    view->setCurrentIndex(m_filterModel->index(0, 0, QModelIndex()));
    view->show();
    createInCombo->setView(view);
}


void
AddBookmarkDialog::accept()
{
    QModelIndex index = createInCombo->view()->currentIndex();
    index = m_filterModel->mapToSource(index);
    if (index.isValid())
    {
        BookmarkItem* folder = m_manager->model()->getItem(index);
        BookmarkItem* newItem = new BookmarkItem(BookmarkItem::Bookmark, folder);
        newItem->setTitle(bookmarkNameEdit->text());
        newItem->setUrl(m_url);
        m_manager->model()->addItem(newItem, folder->childCount());
    }

    QDialog::accept();
}


OrganizeBookmarksDialog::OrganizeBookmarksDialog(BookmarkManager* manager)
{
    setupUi(this);
    
    treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    treeView->setDragEnabled(true);
    treeView->setAcceptDrops(true);
    treeView->setDragDropMode(QAbstractItemView::InternalMove);
    treeView->setDropIndicatorShown(true);
    treeView->setModel(manager->model());
}


void
OrganizeBookmarksDialog::accept()
{
    QDialog::accept();
}
