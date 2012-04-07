#include "MobileImageProvider.hpp"
#include "../core/StelFileMgr.hpp"
#include <QStringBuilder>
#include <QtSvg/QSvgRenderer>
#include <QPainter>
#include <QDebug>

MobileImageProvider::MobileImageProvider(SystemDisplayInfo::DpiBucket bucket) :
	QDeclarativeImageProvider(QDeclarativeImageProvider::Pixmap),
	bucket(bucket)
{

}

QPixmap MobileImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
	QString imagePath = findPath(id, bucket);
	QPixmap pixmap;
	if(imagePath == "")
	{
		qWarning() << "MobileImageProvider couldn't find image " << id;
		pixmap = generatePixmap(findFile(QString(MISSING_IMAGE)), requestedSize);
	}
	else
	{
		pixmap = generatePixmap(imagePath, requestedSize);
		if(pixmap.size() != requestedSize)
		{
			QString scaledImagePath = findFile(DEFAULT_DPI_PREFIX % id, QString(VECTOR_POSTFIX));
			if(scaledImagePath != "")
			{
				pixmap = generatePixmap(scaledImagePath, requestedSize);
			}
		}
	}

	size->setHeight(pixmap.height());
	size->setWidth(pixmap.width());
	return pixmap;
}

QPixmap MobileImageProvider::generatePixmap(QString path, const QSize &requestedSize)
{
	if(path.endsWith(QString(RASTER_POSTFIX), Qt::CaseInsensitive))
	{
		return QPixmap(path);
	}

	Q_ASSERT_X(path.endsWith(QString(VECTOR_POSTFIX), Qt::CaseInsensitive), "MobileImageProvider::generatePixmap", QString("has an invalid path! " % path).toAscii());

	QSvgRenderer svgRenderer(path);
	QPixmap pixmap(requestedSize);
	svgRenderer.render(new QPainter(&pixmap));
	return pixmap;
}

QString MobileImageProvider::findFile(QString stelPath, QString extension)
{
	QString systemPath;
	if(extension != "")
	{
		try
		{
			systemPath = StelFileMgr::findFile(stelPath % extension, StelFileMgr::File);
		}
		catch(std::exception e)
		{
			systemPath = "";
		}

		return systemPath;
	}
	else
	{
		try
		{
			systemPath = StelFileMgr::findFile(stelPath % RASTER_POSTFIX, StelFileMgr::File);
		}
		catch(std::exception e)
		{
			systemPath = "";
		}

		if(systemPath != "")
		{
			return systemPath;
		}

		try
		{
			systemPath = StelFileMgr::findFile(stelPath % VECTOR_POSTFIX, StelFileMgr::File);
		}
		catch(std::exception e)
		{
			systemPath = "";
		}

		return systemPath;
	}
}

QString MobileImageProvider::findPath(const QString &id, SystemDisplayInfo::DpiBucket testBucket)
{
	QString systemPath; //an actual system path
	SystemDisplayInfo::DpiBucket scaledBucket; //second bucket for testing if testBucket fails
	Q_ASSERT_X(testBucket != SystemDisplayInfo::INVALID_DPI, "MobileImageProvider::findFile", "Invalid DPI bucket!");

	// Try the directory for the current bucket

	systemPath = findFile(getBucketPath(testBucket) % id);

	if(systemPath != "")
	{
		return systemPath;
	}

	if(testBucket < SystemDisplayInfo::HIGH_DPI)
	{
		// 1. Try the directory two buckets up

		if(testBucket == SystemDisplayInfo::LOW_DPI)
		{
			scaledBucket = SystemDisplayInfo::HIGH_DPI;
		}
		else
		{
			scaledBucket = SystemDisplayInfo::XHIGH_DPI;
		}

		systemPath = findFile(getBucketPath(scaledBucket) % id);

		if(systemPath != "")
		{
			return systemPath;
		}
	}

	// 2. Look in DEFAULT_DPI_PREFIX for a vector image

	systemPath = findFile(DEFAULT_DPI_PREFIX % id, QString(VECTOR_POSTFIX));

	if(systemPath != "")
	{
		return systemPath;
	}

	// 3. Look in NOSCALE_PREFIX

	systemPath = findFile(NOSCALE_PREFIX % id);

	if(systemPath != "")
	{
		return systemPath;
	}

	// On failing 3., complain. Failing 3. shouldn't happen in ideal circumstances, and
	// probably indicates you don't have enough artwork.
	qWarning() << "MobileImageProvider failed to find an image that will scale nicely. Your UI will look terrible. DPI: " << this->bucket << " image: " << id;

	// 4. Try the directory one bucket up.
	if(testBucket < SystemDisplayInfo::XHIGH_DPI)
	{
		switch(testBucket)
		{
		case SystemDisplayInfo::LOW_DPI:
			scaledBucket = SystemDisplayInfo::MEDIUM_DPI;
			break;
		case SystemDisplayInfo::MEDIUM_DPI:
			scaledBucket = SystemDisplayInfo::HIGH_DPI;
			break;
		case SystemDisplayInfo::HIGH_DPI:
			scaledBucket = SystemDisplayInfo::XHIGH_DPI;
			break;
		default:
			Q_ASSERT_X(false, "MobileImageProvider", "hit an invalid situation in step 4");
		}

		systemPath = findFile(getBucketPath(scaledBucket) % id);

		if(systemPath != "")
		{
			return systemPath;
		}
	}

	// 5. Try the directory three buckets up (if there is one)
	if(testBucket == SystemDisplayInfo::LOW_DPI)
	{
		scaledBucket = SystemDisplayInfo::XHIGH_DPI;

		systemPath = findFile(getBucketPath(scaledBucket) % id);

		if(systemPath != "")
		{
			return systemPath;
		}
	}

	// 6. Look in DEFAULT_DPI_PREFIX for a raster image

	systemPath = findFile(DEFAULT_DPI_PREFIX % id, QString(RASTER_POSTFIX));

	if(systemPath != "")
	{
		return systemPath;
	}

	// 7. Try the lower buckets, and scale the image up. Your GUI will be ugly.
	// (switch fall-through intentional)
	switch(testBucket)
	{
	case SystemDisplayInfo::XHIGH_DPI:
		systemPath = findFile(getBucketPath(SystemDisplayInfo::HIGH_DPI) % id);

		if(systemPath != "")
		{
			break;
		}
	case SystemDisplayInfo::HIGH_DPI:
		systemPath = findFile(getBucketPath(SystemDisplayInfo::MEDIUM_DPI) % id);

		if(systemPath != "")
		{
			break;
		}
	case SystemDisplayInfo::MEDIUM_DPI:
		systemPath = findFile(getBucketPath(SystemDisplayInfo::LOW_DPI) % id);
		break;
	default:
		systemPath = "";
		break;
	}

	return systemPath;
}

QString MobileImageProvider::getBucketPath(SystemDisplayInfo::DpiBucket pathBucket)
{
	switch(pathBucket)
	{
	case SystemDisplayInfo::LOW_DPI:
		return QString(LOW_DPI_PREFIX);
		break;

	case SystemDisplayInfo::MEDIUM_DPI:
		return QString(MEDIUM_DPI_PREFIX);
		break;

	case SystemDisplayInfo::HIGH_DPI:
		return QString(HIGH_DPI_PREFIX);
		break;

	case SystemDisplayInfo::XHIGH_DPI:
		return QString(XHIGH_DPI_PREFIX);
		break;

	default:
		return QString(DEFAULT_DPI_PREFIX);
		break;
	}
}
