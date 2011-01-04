/******************************************************************************
**
** This file is part of libcommhistory.
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Alexander Shalamov <alexander.shalamov@nokia.com>
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of the GNU Lesser General Public License version 2.1 as
** published by the Free Software Foundation.
**
** This library is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
** or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
** License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this library; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
**
******************************************************************************/

#include <QtTest/QtTest>
#include <QtTracker/Tracker>
#include <QtTracker/rdfunbound.h>
#include <QtTracker/ontologies/nmo.h>
#include <QFile>
#include <QTextStream>
#include "eventmodel.h"
#include "groupmodel.h"
#include "event.h"
#include "common.h"
#include "trackerio.h"

using namespace CommHistory;
using namespace SopranoLive;

const int numWords = 23;
const char* msgWords[] = { "lorem","ipsum","dolor","sit","amet","consectetur",
    "adipiscing","elit","in","imperdiet","cursus","lacus","vitae","suscipit",
    "maecenas","bibendum","rutrum","dolor","at","hendrerit",":)",":P","OMG!!" };
int ticks = 0;
int idleTicks = 0;

int addTestEvent(EventModel &model,
                 Event::EventType type,
                 Event::EventDirection direction,
                 const QString &account,
                 int groupId,
                 const QString &text,
                 bool isDraft,
                 bool isMissedCall,
                 const QDateTime &when,
                 const QString &remoteUid,
                 bool toModelOnly,
                 const QString messageToken)
{
    Event event;
    event.setType(type);
    event.setDirection(direction);
    event.setGroupId(groupId);
    event.setStartTime(when);
    event.setEndTime(when);
    event.setLocalUid(account);
    if (remoteUid.isEmpty()) {
        event.setRemoteUid(type == Event::SMSEvent ? "555123456" : "td@localhost");
    } else {
        event.setRemoteUid(remoteUid);
    }
    event.setFreeText(text);
    event.setIsDraft( isDraft );
    event.setIsMissedCall( isMissedCall );
    event.setMessageToken(messageToken);
    if (model.addEvent(event, toModelOnly)) {
        return event.id();
    }
    return -1;
}

void addTestGroups(Group &group1, Group &group2)
{
    addTestGroup(group1,
                 "/org/freedesktop/Telepathy/Account/gabble/jabber/dut_40localhost0",
                 "td@localhost");
    addTestGroup(group2,
                 "/org/freedesktop/Telepathy/Account/gabble/jabber/dut_40localhost0",
                 "td2@localhost");
}

void addTestGroup(Group& grp, QString localUid, QString remoteUid)
{
    GroupModel groupModel;
    grp.setLocalUid(localUid);
    QStringList uids;
    uids << remoteUid;
    grp.setRemoteUids(uids);

    QVERIFY(groupModel.addGroup(grp));

    // wait till group is really added to tracker, so getGroup will not fail in
    // testcases
    bool added = false;
    for (int i=0; i < 5 && !added; i++) {
        Group group;
        added = groupModel.trackerIO().getGroup(grp.id(), group);
        if (!added)
            sleep(1);
    }
}

void addTestContact(const QString &name, const QString &remoteUid)
{
    Live<nco::PersonContact> contact;
    Live<nco::VoicePhoneNumber> number;
    QUrl contactURI;
    QUrl phoneNumberURI;

    phoneNumberURI = QString(QLatin1String("tel:%1")).arg(remoteUid);
    number = ::tracker()->liveNode(phoneNumberURI);

    contactURI = QString(QLatin1String("contact:%1")).arg(remoteUid);
    contact = ::tracker()->liveNode(contactURI);
    contact->setContactUID(remoteUid + remoteUid);
    contact->setContactLocalUID(remoteUid);
    contact->addHasPhoneNumber(number);
    contact->setNameFamily(name);
}

void modifyTestContact(const QString &name, const QString &remoteUid)
{
    Live<nco::PersonContact> contact;
    QUrl contactURI;

    contactURI = QString(QLatin1String("contact:%1")).arg(remoteUid);
    contact = ::tracker()->liveNode(contactURI);
    contact->setNameFamily(name);
}

bool compareEvents(Event &e1, Event &e2)
{
    if (e1.type() != e2.type()) {
        qWarning() << "type:" << e1.type() << e2.type();
        return false;
    }
    if (e1.direction() != e2.direction()) {
        qWarning() << "direction:" << e1.direction() << e2.direction();
        return false;
    }
    if (e1.startTime().toTime_t() != e2.startTime().toTime_t()) {
        qWarning() << "startTime:" << e1.startTime() << e2.startTime();
        return false;
    }
    if (e1.endTime().toTime_t() != e2.endTime().toTime_t()) {
        qWarning() << "endTime:" << e1.endTime() << e2.endTime();
        return false;
    }
    if (e1.isDraft() != e2.isDraft()) {
        qWarning() << "isDraft:" << e1.isDraft() << e2.isDraft();
        return false;
    }
    if (e1.isRead() != e2.isRead()) {
        qWarning() << "isRead:" << e1.isRead() << e2.isRead();
        return false;
    }
    if (e1.isMissedCall() != e2.isMissedCall()) {
        qWarning() << "isMissedCall:" << e1.isMissedCall() << e2.isMissedCall();
        return false;
    }
//    QCOMPARE(e1.bytesSent(), e2.bytesSent());
//    QCOMPARE(e1.bytesReceived(), e2.bytesReceived());
    if (e1.localUid() != e2.localUid()) {
        qWarning() << "localUid:" << e1.localUid() << e2.localUid();
        return false;
    }
    if (e1.remoteUid() != e2.remoteUid()) {
        qWarning() << "remoteUid:" << e1.remoteUid() << e2.remoteUid();
        return false;
    }
    if (e1.freeText() != e2.freeText()) {
        qWarning() << "freeText:" << e1.freeText() << e2.freeText();
        return false;
    }
    if (e1.groupId() != e2.groupId()) {
        qWarning() << "groupId:" << e1.groupId() << e2.groupId();
        return false;
    }
    if (e1.fromVCardFileName() != e2.fromVCardFileName()) {
        qWarning() << "vcardFileName:" << e1.fromVCardFileName() << e2.fromVCardFileName();
        return false;
    }
    if (e1.fromVCardLabel() != e2.fromVCardLabel()) {
        qWarning() << "vcardLabel:" << e1.fromVCardLabel() << e2.fromVCardLabel();
        return false;
    }
//    QCOMPARE(e1.messageToken(), e2.messageToken());
    return true;
}

void deleteAll()
{
    SopranoLive::RDFTransactionPtr transaction;
    transaction = ::tracker()->createTransaction();

    qDebug() << __FUNCTION__ << "- Deleting all messages";
    RDFUpdate messageDeleter;
    messageDeleter.addDeletion(RDFVariable::fromType<nmo::Message>(),
                               rdf::type::iri(), rdfs::Resource::iri());
    ::tracker()->executeQuery(messageDeleter);
    transaction->commitAndReinitiate(true);

    qDebug() << __FUNCTION__ << "- Deleting all calls";
    RDFUpdate callDeleter;
    callDeleter.addDeletion(RDFVariable::fromType<nmo::Call>(),
                            rdf::type::iri(), rdfs::Resource::iri());
    ::tracker()->executeQuery(callDeleter);
    transaction->commitAndReinitiate(true);

    qDebug() << __FUNCTION__ << "- Deleting all groups";
    RDFUpdate groupDeleter;
    groupDeleter.addDeletion(RDFVariable::fromType<nmo::CommunicationChannel>(),
                             rdf::type::iri(), rdfs::Resource::iri());
    ::tracker()->executeQuery(groupDeleter);
    transaction->commitAndReinitiate(true);

    qDebug() << __FUNCTION__ << "- Deleting all contacts";
    RDFUpdate contactDeleter;
    contactDeleter.addDeletion(RDFVariable::fromType<nco::PersonContact>(),
                               rdf::type::iri(), rdfs::Resource::iri());
    ::tracker()->executeQuery(contactDeleter);
    transaction->commitAndReinitiate(true);

    qDebug() << __FUNCTION__ << "- Deleting all phone numbers";
    RDFUpdate numberDeleter;
    numberDeleter.addDeletion(RDFVariable::fromType<nco::PhoneNumber>(),
                              rdf::type::iri(), rdfs::Resource::iri());
    ::tracker()->executeQuery(numberDeleter);
    transaction->commit(true);
}

void deleteSmsMsgs()
{
    RDFUpdate deleter;
    deleter.addDeletion(RDFVariable::fromType<nmo::SMSMessage>(),
                        rdf::type::iri(), rdfs::Resource::iri());
    ::tracker()->executeQuery(deleter);
}

QString randomMessage(int words)
{
    QString msg;
    QTextStream msgStream(&msg, QIODevice::WriteOnly);
    for(int j = 0; j < words; j++) {
        msgStream << msgWords[qrand() % numWords] << " ";
    }
    return msg;
}

/*
 * Returns the average system load since last time this function was called (or
 * since boot if this first time this function is called). The scale is [1, 0],
 * where 1 is busy and 0 is idle system. In case of errors, -1 is returned.
 */
double getSystemLoad()
{
    // Parsing assumes that first line of /proc/stat looks something like this:
    // cpu  110807 325 23491 425840 37435 1367 32 0 0

    QFile file("/proc/stat");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << __PRETTY_FUNCTION__ << "Failed to open /proc/stat";
        return -1;
    }

    QTextStream in(&file);
    QString line = in.readLine();
    file.close();

    QStringList parts = line.split(" ", QString::SkipEmptyParts);
    if (parts.size() != 10) {
        qWarning() << __PRETTY_FUNCTION__ << "Invalid input from /proc/stat:" << line;
        return -1;
    }

    int newIdleTicks = parts.at(4).toInt();
    int newAllTicks = 0;
    for (int i = 1; i < 10; i++) {
        newAllTicks += parts.at(i).toInt();
    }

    int idleTickDelta = newIdleTicks - idleTicks;
    int allTickDelta = newAllTicks - ticks;
    double load = 1.0 - ((double)idleTickDelta / allTickDelta);

    idleTicks = newIdleTicks;
    ticks = newAllTicks;

    return load;
}

/*
 * Wait in semi-busy loop until system load drops below IDLE_TRESHOLD
 */
void waitForIdle(int pollInterval) {
    double load = 1.0;
    QDateTime startTime = QDateTime::currentDateTime();
    getSystemLoad();
    while (load > IDLE_TRESHOLD) {
        qDebug() << __PRETTY_FUNCTION__ << "Waiting system to calm down. Wait time:"
            << startTime.secsTo(QDateTime::currentDateTime()) << "seconds. Load:"
            << load * 100 << "\%";
        QTest::qWait(pollInterval);
        load = getSystemLoad();
    }
    qDebug() << __PRETTY_FUNCTION__ << "Done. Wait time:"
        << startTime.secsTo(QDateTime::currentDateTime()) << "seconds. Load:"
        << load * 100 << "\%";
}

bool waitSignal(QSignalSpy &spy, int msec)
{
    QTime timer;
    timer.start();
    while (timer.elapsed() < msec && spy.isEmpty())
        QCoreApplication::processEvents();

    return !spy.isEmpty();
}
