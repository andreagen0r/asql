/*
 * SPDX-FileCopyrightText: (C) 2020-2022 Daniel Nicoletti <dantti12@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "../../src/acache.h"
#include "../../src/adatabase.h"
#include "../../src/amigrations.h"
#include "../../src/apg.h"
#include "../../src/apool.h"
#include "../../src/apreparedquery.h"
#include "../../src/aresult.h"
#include "../../src/atransaction.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QTimer>
#include <QUrl>
#include <QUuid>

using namespace ASql;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    APool::create(APg::factory(QStringLiteral("postgres:///?target_session_attrs=read-write")));
    APool::setMaxIdleConnections(2);
    APool::setMaxConnections(4);

    {
        APreparedQuery q1(u"SELECT now()");
        APreparedQuery q2(u"SELECT now()");
        Q_ASSERT(q1.identification() != q2.identification());
    }

    auto callBD = []() {
        auto db = APool::database();
        db.exec(APreparedQuery(u"SELECT now()"), nullptr, [=](AResult &result) {
            if (result.error()) {
                qDebug() << "SELECT operator error" << result.errorString();
                return;
            }

            qDebug() << "PREPARED operator size" << result.toHashList();
        });
    };

    callBD();

    callBD();

    auto simpleDb = APool::database();
    simpleDb.exec(APreparedQuery(u"SELECT $1, now()"), {qint64(12345)}, nullptr, [=](AResult &result) {
        if (result.error()) {
            qDebug() << "SELECT error" << result.errorString();
            return;
        }

        qDebug() << "PREPARED size" << result.size();
    });
    simpleDb.exec(APreparedQueryLiteral(u"SELECT broken"), {qint64(12345)}, nullptr, [=](AResult &result) {
        if (result.error()) {
            qDebug() << "SELECT broken error" << result.errorString();
            return;
        }

        qDebug() << "PREPARED broken size" << result.size();
    });
    return app.exec();

    {
        auto db2 = APool::database();
        auto db3 = APool::database();
        auto db4 = APool::database();
        auto db5 = APool::database();
        auto db6 = APool::database();
        auto db7 = APool::database();
        qDebug() << "db7 valid" << db7.isValid();
        ATransaction t(db7);
        t.begin();
        db7.exec(QStringLiteral("SELECT now()"), nullptr, [=](AResult &result) {
            if (result.error()) {
                qDebug() << "SELECT error db7" << result.errorString();
                return;
            }

            if (result.size()) {
                qDebug() << "SELECT value" << result.begin().value(0);
                ATransaction(t).commit();
            }
        });

        //        ADatabase().rollback(); assert

        APool::database(nullptr, [=](ADatabase db) {
            qDebug() << "Got db" << db.isOpen() << db.state();

            db.exec(QStringLiteral("SELECT now()"), nullptr, [=](AResult &result) {
                if (result.error()) {
                    qDebug() << "got db, SELECT error" << result.errorString();
                    return;
                }

                if (result.size()) {
                    qDebug() << "got db, SELECT value" << result.begin().value(0);
                    ATransaction(t).commit();
                }
            });
            qDebug() << "Got db2" << db.isOpen() << db.state();
        });
    }

    auto db = APool::database();
    static APreparedQuery query(QStringLiteral("SELECT now()"));
    db.exec(query, nullptr, [=](AResult &result) {
        if (result.error()) {
            qDebug() << "SELECT 1 error" << result.errorString();
            return;
        }

        if (result.size()) {
            qDebug() << "SELECT 1 value" << result.begin().value(0);
        }
    });

    db.exec(query, nullptr, [=](AResult &result) {
        if (result.error()) {
            qDebug() << "SELECT error" << result.errorString();
            return;
        }

        if (result.size()) {
            qDebug() << "SELECT value" << result.begin().value(0);
        }
    });

    static APreparedQuery query2(QStringLiteral("SELECT now(), $1"));
    db.exec(query2, {qint64(12345)}, nullptr, [=](AResult &result) {
        if (result.error()) {
            qDebug() << "SELECT error" << result.errorString();
            return;
        }

        if (result.size()) {
            qDebug() << "SELECT value 2" << result.begin().value(0) << result.begin().value(1) << query2.identification();
        }
    });

    db.exec(query2, {qint64(12345)}, nullptr, [=](AResult &result) {
        if (result.error()) {
            qDebug() << "SELECT error" << result.errorString();
            return;
        }

        if (result.size()) {
            qDebug() << "SELECT value 2" << result.begin().value(0) << result.begin().value(1) << query2.identification();
        }
    });

    auto loopFn = [=] {
        auto queryStatic = APreparedQueryLiteral(u"SELECT $1, now()");
        ADatabase(db).exec(queryStatic,
                           {qint64(12345)},
                           nullptr,
                           [=](AResult &result) {
            if (result.error()) {
                qDebug() << "SELECT error" << result.errorString();
                return;
            }

            if (result.size()) {
                qDebug() << "SELECT value 3" << result.begin().value(0) << result.begin().value(1) << queryStatic.identification();
            }
        });
    };

    loopFn();
    loopFn();

    app.exec();
}
