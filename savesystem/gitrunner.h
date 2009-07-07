
#ifndef	GITRUNNER_H
#define	GITRUNNER_H

#include	<QObject>
#include	<KUrl>
#include	<QStringList>
#include	<QDir>
#include	<KProcess>

#include	"dvcsjob.h"

class GitRunner
{
	public:
		GitRunner();
		~GitRunner();

		bool isValidDirectory(const KUrl &dirPath);
		bool isRunning();

		void setDirectory(const QDir &dir );
		void setCommunicationMode(KProcess::OutputChannelMode comm);

		DvcsJob::JobStatus add(const KUrl::List& localLocations);
		DvcsJob::JobStatus createWorkingCopy(const KUrl& repoOrigin, const KUrl& repoDestination);
		DvcsJob::JobStatus remove(const KUrl::List& files);
		DvcsJob::JobStatus status(const KUrl &dirPath);
		DvcsJob::JobStatus commit(const QString& message);
		DvcsJob::JobStatus log(const KUrl& localLocation);
		DvcsJob::JobStatus annotate(const KUrl &localLocation );
		DvcsJob::JobStatus init(const KUrl & directory);
		DvcsJob::JobStatus reset(const QStringList &args);
		DvcsJob::JobStatus newBranch(const QString &newBranch);
		DvcsJob::JobStatus switchBranch(const QString &newBranch);
		DvcsJob::JobStatus branches();

		QString& getResult();

	private:
		void resetJob();
		void startJob();

		QString						*m_currentBranch;
		QString						m_result;
		KUrl						*m_lastRepoRoot;
		DvcsJob						*m_job;
		KProcess::OutputChannelMode	m_commMode;
		volatile bool				m_isRunning;
};

#endif
