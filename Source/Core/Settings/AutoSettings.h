// AutoSettings.h

#pragma once

#include <functional>
#include <QObject>
#include <QMetaProperty>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QMap>
#include <QList>


class JsonSettingsNode : public QObject
{
    Q_OBJECT

public:

    explicit JsonSettingsNode(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    virtual ~JsonSettingsNode() = default;

    //////////////////////////////////////////////////////////////
    // REGISTER CHILD NODE
    //////////////////////////////////////////////////////////////

    void registerChild(
        const QString& name,
        JsonSettingsNode* child
    )
    {
        m_children[name] = child;
    }

    //////////////////////////////////////////////////////////////
    // REGISTER ARRAY OF OBJECTS
    //////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////
    // REGISTER CONTAINER
    //
    // Поддерживает:
    // - QList<T>
    // - QVector<T>
    //
    // T должен наследовать JsonSettingsNode
    //////////////////////////////////////////////////////////////

    template<
        template<typename...> class Container,
        typename T
    >
    void registerContainer(
        const QString& name,
        Container<T>* container
    )
    {
        static_assert(
            std::is_base_of<JsonSettingsNode, T>::value,
            "T must inherit JsonSettingsNode"
        );

        ArrayHandler handler;

        //////////////////////////////////////////////////////////
        // SERIALIZE
        //////////////////////////////////////////////////////////

        handler.serialize =
            [container]() -> QJsonArray
            {
                QJsonArray jsonArray;

                for (const auto& item : *container)
                {
                    jsonArray.append(
                        item.toJson()
                    );
                }

                return jsonArray;
            };

        //////////////////////////////////////////////////////////
        // DESERIALIZE
        //////////////////////////////////////////////////////////

        handler.deserialize =
            [container](const QJsonArray& jsonArray)
            {
                container->clear();

                for (const auto& value : jsonArray)
                {
                    if (!value.isObject())
                        continue;

                    T item;

                    item.fromJson(
                        value.toObject()
                    );

                    container->append(item);
                }
            };

        m_arrays[name] = handler;
    }

    //////////////////////////////////////////////////////////////
    // TO JSON
    //////////////////////////////////////////////////////////////

    virtual QJsonObject toJson() const
    {
        QJsonObject json;

        serializeProperties(json);
        serializeChildren(json);
        serializeArrays(json);

        return json;
    }

    //////////////////////////////////////////////////////////////
    // FROM JSON
    //////////////////////////////////////////////////////////////

    virtual void fromJson(
        const QJsonObject& json
    )
    {
        deserializeProperties(json);
        deserializeChildren(json);
        deserializeArrays(json);
    }

    //////////////////////////////////////////////////////////////
    // SAVE
    //////////////////////////////////////////////////////////////

    bool save(const QString& filePath) const
    {
        QFile file(filePath);

        if (!file.open(QIODevice::WriteOnly))
            return false;

        QJsonDocument doc(toJson());

        file.write(
            doc.toJson(QJsonDocument::Indented)
        );

        return true;
    }

    //////////////////////////////////////////////////////////////
    // LOAD
    //////////////////////////////////////////////////////////////

    bool load(const QString& filePath)
    {
        QFile file(filePath);

        if (!file.open(QIODevice::ReadOnly))
            return false;

        QJsonParseError error;

        QJsonDocument doc =
            QJsonDocument::fromJson(
                file.readAll(),
                &error
            );

        if (error.error !=
            QJsonParseError::NoError)
        {
            return false;
        }

        if (!doc.isObject())
            return false;

        fromJson(doc.object());

        return true;
    }

protected:

    //////////////////////////////////////////////////////////////
    // SERIALIZE PROPERTIES
    //////////////////////////////////////////////////////////////

    virtual void serializeProperties(
        QJsonObject& json
    ) const
    {
        const QMetaObject* meta =
            metaObject();

        for (int i = meta->propertyOffset();
             i < meta->propertyCount();
             ++i)
        {
            QMetaProperty prop =
                meta->property(i);

            if (!prop.isReadable())
                continue;

            QString name = prop.name();

            QVariant value =
                prop.read(this);

            json[name] =
                QJsonValue::fromVariant(value);
        }
    }

    //////////////////////////////////////////////////////////////
    // DESERIALIZE PROPERTIES
    //////////////////////////////////////////////////////////////

    virtual void deserializeProperties(
        const QJsonObject& json
    )
    {
        const QMetaObject* meta =
            metaObject();

        for (int i = meta->propertyOffset();
             i < meta->propertyCount();
             ++i)
        {
            QMetaProperty prop =
                meta->property(i);

            if (!prop.isWritable())
                continue;

            QString name = prop.name();

            if (!json.contains(name))
                continue;

            prop.write(
                this,
                json[name].toVariant()
            );
        }
    }

    //////////////////////////////////////////////////////////////
    // SERIALIZE CHILDREN
    //////////////////////////////////////////////////////////////

    virtual void serializeChildren(
        QJsonObject& json
    ) const
    {
        for (auto it = m_children.begin();
             it != m_children.end();
             ++it)
        {
            json[it.key()] =
                it.value()->toJson();
        }
    }

    //////////////////////////////////////////////////////////////
    // DESERIALIZE CHILDREN
    //////////////////////////////////////////////////////////////

    virtual void deserializeChildren(
        const QJsonObject& json
    )
    {
        for (auto it = m_children.begin();
             it != m_children.end();
             ++it)
        {
            if (!json.contains(it.key()))
                continue;

            if (!json[it.key()].isObject())
                continue;

            it.value()->fromJson(
                json[it.key()].toObject()
            );
        }
    }

    //////////////////////////////////////////////////////////////
    // SERIALIZE ARRAYS
    //////////////////////////////////////////////////////////////

    virtual void serializeArrays(
        QJsonObject& json
    ) const
    {
        for (auto it = m_arrays.begin();
             it != m_arrays.end();
             ++it)
        {
            json[it.key()] =
                it.value().serialize();
        }
    }

    //////////////////////////////////////////////////////////////
    // DESERIALIZE ARRAYS
    //////////////////////////////////////////////////////////////

    virtual void deserializeArrays(
        const QJsonObject& json
    )
    {
        for (auto it = m_arrays.begin();
             it != m_arrays.end();
             ++it)
        {
            if (!json.contains(it.key()))
                continue;

            if (!json[it.key()].isArray())
                continue;

            it.value().deserialize(
                json[it.key()].toArray()
            );
        }
    }

private:

    //////////////////////////////////////////////////////////////
    // ARRAY HANDLER
    //////////////////////////////////////////////////////////////

    struct ArrayHandler
    {
        std::function<QJsonArray()> serialize;

        std::function<void(const QJsonArray&)> deserialize;
    };

    //////////////////////////////////////////////////////////////
    // CHILD OBJECTS
    //////////////////////////////////////////////////////////////

    QMap<
        QString,
        JsonSettingsNode*
    > m_children;

    //////////////////////////////////////////////////////////////
    // ARRAYS
    //////////////////////////////////////////////////////////////

    QMap<
        QString,
        ArrayHandler
    > m_arrays;
};
class RecentFile : public JsonSettingsNode
{
    Q_OBJECT

    Q_PROPERTY(QString path MEMBER path)
    Q_PROPERTY(QString name MEMBER name)

public:

    QString path;
    QString name;
};


class GlobalSettings : public JsonSettingsNode
{
    Q_OBJECT

public:

    QVector<RecentFile> recentFiles;

    GlobalSettings()
    {
        registerContainer(
            "recentFiles",
            &recentFiles
        );
    }
};