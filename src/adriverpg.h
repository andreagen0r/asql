/* 
 * SPDX-FileCopyrightText: (C) 2020 Daniel Nicoletti <dantti12@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef ADRIVERPG_H
#define ADRIVERPG_H

#include <adriver.h>
#include <libpq-fe.h>

#include "aresult.h"
#include "apreparedquery.h"

#include <QQueue>
#include <QPointer>
#include <QHash>

typedef struct pg_conn PGconn;

class AResultPg : public AResultPrivate
{
public:
    AResultPg();
    virtual ~AResultPg();

    bool lastResulSet() const override;
    bool error() const override;
    QString errorString() const override;

    int size() const override;
    int fields() const override;
    int numRowsAffected() const override;

    QString fieldName(int column) const override;
    QVariant value(int row, int column) const override;

    bool toBool(int row, int column) const override;
    int toInt(int row, int column) const override;
    qint64 toLongLong(int row, int column) const override;
    quint64 toULongLong(int row, int column) const override;
    double toDouble(int row, int column) const override;
    QString toString(int row, int column) const override;
    QDate toDate(int row, int column) const override;
    QTime toTime(int row, int column) const override;
    QDateTime toDateTime(int row, int column) const override;
    QByteArray toByteArray(int row, int column) const override;

    void processResult();

    QString m_errorString;
    PGresult *m_result = nullptr;
    bool m_error = false;
    bool m_lastResultSet = true;
};

class APGQuery
{
public:
    APGQuery() : result(QSharedPointer<AResultPg>(new AResultPg))
    { }
    QByteArray query;
    APreparedQuery preparedQuery;
    QSharedPointer<AResultPg> result;
    QVariantList params;
    AResultFn cb;
    QSharedPointer<ADatabasePrivate> db;
    QPointer<QObject> receiver;
    QObject *checkReceiver;
    bool preparing = false;
    bool prepared = false;
    bool setSingleRow = false;

    inline void done() {
        AResult r(result);
        if (cb && (!checkReceiver || !receiver.isNull())) {
            cb(r);
        }
    }
};

class ADriverPg final : public ADriver
{
    Q_OBJECT
public:
    ADriverPg();
    virtual ~ADriverPg();

    void open(std::function<void(bool isOpen, const QString &error)> cb) override;
    bool isOpen() const override;

    void setState(ADatabase::State state, const QString &status);
    ADatabase::State state() const override;
    void onStateChanged(std::function<void(ADatabase::State state, const QString &status)> cb) override;

    void begin(QSharedPointer<ADatabasePrivate> db, AResultFn cb, QObject *receiver) override;
    void commit(QSharedPointer<ADatabasePrivate> db, AResultFn cb, bool now, QObject *receiver) override;
    void rollback(QSharedPointer<ADatabasePrivate> db, AResultFn cb, bool now, QObject *receiver) override;

    void exec(QSharedPointer<ADatabasePrivate> db, const QString &query, const QVariantList &params, AResultFn cb, QObject *receiver) override;
    void exec(QSharedPointer<ADatabasePrivate> db, QStringView query, const QVariantList &params, AResultFn cb, QObject *receiver) override;
    void exec(QSharedPointer<ADatabasePrivate> db, const APreparedQuery &query, const QVariantList &params, AResultFn cb, QObject *receiver) override;

    void setLastQuerySingleRowMode() override;

    void subscribeToNotification(QSharedPointer<ADatabasePrivate> db, const QString &name) override;
    void onNotification(QSharedPointer<ADatabasePrivate> db, ANotificationFn cb, QObject *receiver) override;
    QStringList subscribedToNotifications() const override;
    void unsubscribeFromNotification(QSharedPointer<ADatabasePrivate> db, const QString &name, QObject *receiver) override;

private:
    inline void queryConstructed(APGQuery &pgQuery);
    void nextQuery();
    void finishConnection();
    void finishQueries(const QString &error);
    inline void doExec(APGQuery &pgQuery);
    inline void doExecParams(APGQuery &query);
    inline void setSingleRowMode();
    inline void cmdFlush();

    PGconn *m_conn = nullptr;
    ADatabase::State m_state = ADatabase::Disconnected;
    bool m_connected = false;
    bool m_flush = false;
    bool m_queryRunning = false;
    bool m_notificationPtrSet = false;
    std::function<void (ADatabase::State, const QString &)> m_stateChangedCb;
    ANotificationFn m_notificationFn;
    QPointer<QObject> m_notificationPtr;
    QStringList m_subscribedNotifications;
    QQueue<APGQuery> m_queuedQueries;
    QSocketNotifier *m_writeNotify = nullptr;
    QSocketNotifier *m_readNotify = nullptr;
    QByteArrayList m_preparedQueries;
};

#endif // ADRIVERPG_H
