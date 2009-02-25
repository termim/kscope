/***************************************************************************
 *   Copyright (C) 2007-2009 by Elad Lahav
 *   elad_lahav@users.sourceforge.net
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 ***************************************************************************/

#include <QDebug>
#include "lexerstylemodel.h"
#include "config.h"

namespace KScope
{

namespace Editor
{

const QString LexerStyleModel::inheritValue_ = "<Inherit>";

/**
 * Class constructor.
 * @param  parent Parent object
 */
LexerStyleModel::LexerStyleModel(const Config::LexerList& lexers,
                                 const QSettings& settings,
                                 QObject* parent)
	: QAbstractItemModel(parent), root_(NULL)
{
	// Build the style tree from the list of lexers.
	// The first object is assumed to be the common defaults lexer.
	Node* defNode = NULL;
	foreach (QsciLexer* lexer, lexers) {
		if (defNode == NULL) {
			// Create the root node.
			defNode = createStyleNode(&root_, lexer);
			loadStyle(settings, defNode);
		}
		else {
			// Create the default style node for this lexer.
			Node* lexNode = createStyleNode(defNode, lexer);
			loadStyle(settings, lexNode);

			// Create per-style nodes for this lexer.
			for (int i = 0; !lexer->description(i).isEmpty(); i++) {
				// Skip the default style.
				if (i == lexer->defaultStyle())
					continue;

				Node* styleNode = createStyleNode(lexNode, lexer, i);
				loadStyle(settings, styleNode);
			}
		}
	}
}

/**
 * Class destructor.
 */
LexerStyleModel::~LexerStyleModel()
{
}

/**
 * Restores the default styles.
 */
void LexerStyleModel::resetStyles()
{
#if 0
	for (int i = 0; i < styleNum_; i++) {
		lexer_->setFont(lexer_->defaultFont(i), i);
		lexer_->setColor(lexer_->defaultColor(i), i);
		lexer_->setPaper(lexer_->defaultPaper(i), i);
	}

	dynamic_cast<LexerExInterface*>(lexer_)->setUseGlobalFont(false);
#endif

	reset();
}

/**
 * Creates an index.
 * @param  row    The row of the index, relative to the parent
 * @param  column The column of the index
 * @param  parent The parent index.
 * @return The resulting index
 */
QModelIndex LexerStyleModel::index(int row, int column,
                                   const QModelIndex& parent) const
{
	const Node* node = nodeFromIndex(parent);
	if (node == NULL)
		return QModelIndex();

	// Root nodes do not have data.
	if (node->data() == NULL)
		return createIndex(row, column, (void*)node->child(row));

	// Property nodes do not have children.
	if (node->data()->type() == PropertyNode)
		return QModelIndex();

	// For column 2 on a style node, return an index representing the root
	// property node.
	if (column == 2) {
		StyleData* style = static_cast<StyleData*>(node->data());
		return createIndex(row, column, (void*)&style->propRoot_);
	}

	// Return an index representing a child style.
	return createIndex(row, column, (void*)node->child(row));
}

/**
 * Finds the parent for the given index.
 * @param  index The index for which the parent is needed
 * @return The parent index
 */
QModelIndex LexerStyleModel::parent(const QModelIndex& index) const
{
	if (!index.isValid())
		return QModelIndex();

	const Node* node = nodeFromIndex(index);
	if (node == NULL)
		return QModelIndex();

	// Handle root nodes.
	if (node->data() == NULL)
		return QModelIndex();

	// Handle top-level style nodes.
	if (node->parent() == &root_)
		return QModelIndex();

	return createIndex(node->parent()->index(), 0, node->parent());
}

/**
 * Determines the number of child indices for the given parent.
 * @param  parent The parent index
 * @return The number of children for the index
 */
int LexerStyleModel::rowCount(const QModelIndex& parent) const
{
	const Node* node = nodeFromIndex(parent);
	if (node == NULL)
		return 0;

	// Handle root nodes.
	if (node->data() == NULL)
		return node->childCount();

	// Property nodes have no children.
	if (node->data()->type() == PropertyNode)
		return 0;

	// Column 2 is used for editing properties, so the number of children
	// is the number of available properties.
	if (parent.column() == 2) {
		StyleData* data = static_cast<StyleData*>(node->data());
		return data->propRoot_.childCount();
	}

	return node->childCount();
}

/**
 * Determines the number of columns in children of the given index.
 * This number is always 2.
 * @param  parent Ignored
 * @return The number of columns for children of the index
 */
int LexerStyleModel::columnCount(const QModelIndex& parent) const
{
	(void)parent;
	return 2;
}

/**
 * Provides the data to display/edit for a given index and role.
 * @param  index The index for which data is requested
 * @param  role  The requested role
 * @return The relevant data
 */
QVariant LexerStyleModel::data(const QModelIndex& index, int role) const
{
	const Node* node = nodeFromIndex(index);
	if (node == NULL || node->data() == NULL)
		return 0;

	if (node->data()->type() == StyleNode) {
		// Get the lexer and style ID for this node.
		StyleData* data = static_cast<StyleData*>(node->data());
		QsciLexer* lexer = data->lexer_;
		int style = data->style_;

		switch (index.column()) {
		case 0:
			// Show language name or style name in the first column.
			if (role == Qt::DisplayRole) {
				if (style == lexer->defaultStyle())
					return lexer->language();

				return lexer->description(style);
			}
			break;

		case 1:
			// Show a formatted text string in the second column, using the
			// style's properties.
			return styleData(node, role);
		}
	}
	else {
		// Get the lexer and style ID for this node.
		PropertyData* data = static_cast<PropertyData*>(node->data());

		switch (index.column()) {
		case 0:
			if (role == Qt::DisplayRole)
				return propertyName(data->prop_);
			break;

		case 1:
			return propertyData(data, role);
		}
	}

	return QVariant();
}

/**
 * Modifies a style's property.
 * @param index
 * @param value
 * @param role
 * @return
 */
bool LexerStyleModel::setData(const QModelIndex& index, const QVariant& value,
                              int role)
{
	// Can only modify property nodes.
	Node* node = nodeFromIndex(index);
	if (!node || !node->data() || !node->data()->type() == PropertyNode)
		return false;

	if (role != Qt::EditRole)
		return false;

	// Set the property's value.
	PropertyData* data = static_cast<PropertyData*>(node->data());
	Node* styleNode = data->styleNode_;
	setProperty(value, styleNode, data->prop_, QVariant());

	// Update changes.
	emit dataChanged(index, index);
	QModelIndex styleIndex = createIndex(styleNode->index(), 1, styleNode);
	emit dataChanged(styleIndex, styleIndex);

	// Apply property to inheriting styles.
	if (value != inheritValue_)
		inheritProperty(value, styleNode, data->prop_);

	return true;
}

Qt::ItemFlags LexerStyleModel::flags(const QModelIndex& index) const
{
	const Node* node = nodeFromIndex(index);
	if (!node || !node->data())
		return Qt::NoItemFlags;

	Qt::ItemFlags flags = Qt::NoItemFlags;

	switch (node->data()->type()) {
	case StyleNode:
		flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
		break;

	case PropertyNode:
		flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
		if (index.column() == 1)
			flags |= Qt::ItemIsEditable;
		break;
	}

	return flags;
}

LexerStyleModel::Node* LexerStyleModel::createStyleNode(Node* parent,
                                                        QsciLexer* lexer,
                                                        int style)
{
	// Create the node.
	StyleData* data = new StyleData(lexer, style);
	Node* node = parent->addChild(data);

	// Create style properties.
	for (uint i = 0; i != _LastProperty; i++) {
		PropertyData* prop = new PropertyData(static_cast<StyleProperty>(i));
		prop->styleNode_ = node;
		data->propRoot_.addChild(prop);
	}

	return node;
}

/**
 * Reads style data from a QSettings object.
 * If data is not available, the method attempts to provide good defaults based
 * on the current lexer settings.
 * @param  settings The QSettings object to read from
 * @param  node     The style node
 */
void LexerStyleModel::loadStyle(const QSettings& settings, Node* node)
{
	// Get the lexer and style ID from the node data.
	StyleData* data = static_cast<StyleData*>(node->data());
	QsciLexer* lexer = data->lexer_;
	int style = data->style_;

	// Create a key template for the settings object, of the form
	// LEXER\STYLE\%1, where %1 will be replaced by the property name.
	QString key = QString("%1\\%2\\%3").arg(lexer->lexer())
	                                   .arg(lexer->description(style));

	// Get the properties.
	setProperty(settings.value(key.arg("Font")), node, Font,
	            lexer->font(style));
	setProperty(settings.value(key.arg("Foreground")), node, Foreground,
	            lexer->color(style));
	setProperty(settings.value(key.arg("Background")), node, Background,
	            lexer->paper(style));
}

/**
 * Assigns a value to a property.
 * The val parameter that is passed to this method can have one of three types
 * of values:
 * 1. A value that matches to the type of the property,
 * 2. The special string "<Inherit>",
 * 3. An invalid QVariant.
 * In the first case, the given value is set to the property. In the second, the
 * property value is set to that of the matching property as defined by the
 * parent style.
 * The third case happens when a property value is not defined (e.g., when
 * reading from an initial settings file). The property is set to the lexer's
 * value for that style, as passed in the defVal parameter. The method then
 * guesses whether the value is inherited, by comparing the value with that
 * of the parent style. This should give a good out-of-the-box default
 * behaviour.
 * @param  val    The value to assign
 * @param  node   The style node that owns the property
 * @param  prop   Property ID
 * @param  defVal A default value to use
 */
void LexerStyleModel::setProperty(const QVariant& val, Node* node,
                                  StyleProperty prop, const QVariant& defVal)
{
	PropertyData* data = propertyDataFromNode(node, prop);

	// Assign the value if its type matches the property type.
	if (val.type() == propertyType(prop)) {
		data->value_ = val;
		data->inherited_ = false;
		return;
	}

	// The root style cannot inherit values.
	// Use the default value, if it was properly defined. Otherwise, leave the
	// current value.
	if (node->parent() == &root_) {
		if (defVal.type() == propertyType(prop))
			data->value_ = defVal;
		data->inherited_ = false;
		return;
	}

	// Determine whether the value should be inherited.
	if (!isInheritValue(val)) {
		// No value was found in the settings object, and the property is
		// not marked as inherited.
		// Make an educated guess on whether the property should be
		// inherited, by checking if the parent value is the same as the
		// default one.
		if ((defVal.type() == propertyType(prop))
		    && (propertyDataFromNode(node->parent(), prop)->value_ != defVal)) {
			// Parent value differs, use the default one.
			data->value_ = defVal;
			data->inherited_ = false;
			return;
		}
	}

	// The value should be inherited.
	// Get the parent style's value for this property.
	data->value_ = propertyDataFromNode(node->parent(), prop)->value_;
	data->inherited_ = true;
}

/**
 * Recursively applies a property to all inheriting styles.
 * @param  val  The new property value
 * @param  node The parent style node
 * @param  prop The property to set
 */
void LexerStyleModel::inheritProperty(const QVariant& val, Node* node,
                                      StyleProperty prop)
{
	for (int i = 0; i < node->childCount(); i++) {
		// Get the child node information.
		Node* child = node->child(i);
		PropertyData* data = propertyDataFromNode(child, prop);

		// Check if this property is inherited by the child.
		if (data->inherited_) {
			// Set the new value.
			data->value_ = val;

			// Notify views of the change.
			QModelIndex index = createIndex(i, 1, (void*)child);
			emit dataChanged(index, index);

			// Recursive application.
			inheritProperty(val, child, prop);
		}
	}
}

/**
 * Creates a string with the style's font and colours to be displayed for the
 * second column of a style item.
 * @param  node The style node
 * @param  role The role to use
 * @return The data for the given style and role
 */
QVariant LexerStyleModel::styleData(const Node* node, int role) const
{
	switch (role) {
	case Qt::DisplayRole:
		return QString("Sample Text");

	case Qt::FontRole:
		return propertyDataFromNode(node, Font)->value_;

	case Qt::ForegroundRole:
		return propertyDataFromNode(node, Foreground)->value_;

	case Qt::BackgroundRole:
		return propertyDataFromNode(node, Background)->value_;

	default:
		;
	}

	return QVariant();
}

/**
 * @param  prop Property value
 * @return A display name for this property
 */
QString LexerStyleModel::propertyName(StyleProperty prop) const
{
	switch (prop) {
	case Font:
		return tr("Font");

	case Foreground:
		return tr("Text Colour");

	case Background:
		return tr("Background Colour");

	default:
		// Must not get here.
		Q_ASSERT(false);
	}

	return QString();
}

/**
 * @param  prop Property value
 * @return The QVariant type that is used to hold values for this property
 */
QVariant::Type LexerStyleModel::propertyType(StyleProperty prop) const
{
	switch (prop) {
	case Font:
		return QVariant::Font;

	case Foreground:
		return QVariant::Color;

	case Background:
		return QVariant::Color;

	default:
		// Must not get here.
		Q_ASSERT(false);
	}

	return QVariant::Invalid;
}

/**
 * @param  data Property data
 * @param  role The role to use
 * @return The value to return for this property
 */
QVariant LexerStyleModel::propertyData(PropertyData* data, int role) const
{
	switch (role) {
	case Qt::DisplayRole:
		if (data->inherited_)
			return tr("Inherit");
		if (propertyType(data->prop_) == QVariant::Font)
			return data->value_;
		break;

	case Qt::DecorationRole:
		if ((!data->inherited_)
			&& (propertyType(data->prop_) == QVariant::Color)) {
			return data->value_;
		}
		break;

	case Qt::EditRole:
		return data->value_;
	}

	return QVariant();
}

} // namespace Editor

} // namespace KScope
