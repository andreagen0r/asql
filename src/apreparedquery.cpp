/* 
 * SPDX-FileCopyrightText: (C) 2020 Daniel Nicoletti <dantti12@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "apreparedquery.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(ASQL_PQ, "asql.prepared_query", QtInfoMsg)

using namespace ASql;

APreparedQuery::APreparedQuery() = default;

static QByteArray identificationCounter() {
    QByteArray ret;
    static QBasicAtomicInt qPreparedStmtCount = Q_BASIC_ATOMIC_INITIALIZER(0);
    ret = "asql_" + QByteArray::number(qPreparedStmtCount.fetchAndAddRelaxed(1) + 1, 16);
    qDebug(ASQL_PQ) << "Created prepared query identifier" << ret;
    return ret;
}

APreparedQuery::APreparedQuery(QStringView query)
    : m_query(query.toUtf8())
    , m_identification(identificationCounter())
{
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
APreparedQuery::APreparedQuery(QUtf8StringView query)
    : m_query(query.data(), query.size())
    , m_identification(identificationCounter())
{
}
#endif

APreparedQuery::APreparedQuery(QStringView query, QStringView identification)
    : m_query(query.toUtf8())
    , m_identification(identification.toUtf8())
{
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
APreparedQuery::APreparedQuery(QUtf8StringView query, QUtf8StringView identification)
    : m_query(query.data(), query.size())
    , m_identification(identification.data(), identification.size())
{
}
#endif

QByteArray APreparedQuery::query() const
{
    return m_query;
}

QByteArray APreparedQuery::identification() const
{
    return m_identification;
}
