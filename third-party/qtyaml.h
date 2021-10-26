/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ***** END LICENSE BLOCK ***** */

/*
 * Copyright (c) 2014, Filip Brcic <brcha@gna.org>. All rights reserved.
 *
 * This file is part of lusim
 */

#ifndef QTYAML_H
#define QTYAML_H

#include <yaml-cpp/yaml.h>

#include <QtCore/QString>
#include <QtCore/QMap>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QPair>

namespace YAML {

// QString
template<>
struct convert<QString>
{
    static Node encode(const QString& rhs)
    {
        return Node(rhs.toStdString());
    }

    static bool decode(const Node& node, QString& rhs)
    {
        if (!node.IsScalar())
            return false;
        rhs = QString::fromStdString(node.Scalar());
        return true;
    }
};

// QMap
template<typename Key, typename Value>
struct convert<QMap<Key, Value> >
{
    static Node encode(const QMap<Key,Value>& rhs)
    {
        Node node(NodeType::Map);
        auto it = rhs.constBegin();
        while (it != rhs.constEnd())
        {
            node.force_insert(it.key(), it.value());
            ++it;
        }
        return node;
    }

    static bool decode(const Node& node, QMap<Key, Value>& rhs)
    {
        if (!node.IsMap())
            return false;

        rhs.clear();
        const_iterator it = node.begin();
        while (it != node.end())
        {
            rhs[it->first.as<Key>()] = it->second.as<Value>();
            ++it;
        }
        return true;
    }
};

// QVector
template<typename T>
struct convert<QVector<T> >
{
    static Node encode(const QVector<T>& rhs)
    {
        Node node(NodeType::Sequence);
        foreach (T value, rhs) {
            node.push_back(value);
        }
        return node;
    }

    static bool decode(const Node& node, QVector<T>& rhs)
    {
        if (!node.IsSequence())
            return false;

        rhs.clear();
        const_iterator it = node.begin();
        while (it != node.end())
        {
            rhs.push_back(it->as<T>());
            ++it;
        }
        return true;
    }
};

// QStringList
template<>
struct convert<QStringList>
{
    static Node encode(const QStringList& rhs)
    {
        Node node(NodeType::Sequence);
        for (const auto &value: rhs) {
            node.push_back(value);
        }
        return node;
    }

    static bool decode(const Node& node, QStringList& rhs)
    {
        if (!node.IsSequence())
            return false;

        rhs.clear();
        const_iterator it = node.begin();
        while (it != node.end())
        {
            rhs.push_back(it->as<QString>());
            ++it;
        }
        return true;
    }
};

// TODO: Add the rest of the container classes
// QLinkedList, QStack, QQueue, QSet, QMultiMap, QHash, QMultiHash, QStringList, ...

} // end namespace YAML

#endif // QTYAML_H
