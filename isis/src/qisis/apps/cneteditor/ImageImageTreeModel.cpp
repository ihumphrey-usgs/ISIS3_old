#include "IsisDebug.h"

#include "ImageImageTreeModel.h"

#include <iostream>

#include <QFuture>
#include <QFutureWatcher>
#include <QList>
#include <QString>
#include <QtConcurrentMap>

#include "ControlCubeGraphNode.h"
#include "ControlMeasure.h"
#include "ControlNet.h"

#include "TreeView.h"
#include "TreeViewContent.h"
#include "PointLeafItem.h"
#include "RootItem.h"
#include "ImageLeafItem.h"
#include "ImageParentItem.h"


#include <QTime>

using std::cerr;


namespace Isis
{
  namespace CnetViz
  {
    ImageImageTreeModel::ImageImageTreeModel(ControlNet * cNet, TreeView * v,
        QObject * parent) : AbstractTreeModel(cNet, v, parent)
    {
      rebuildItems();
    }


    ImageImageTreeModel::~ImageImageTreeModel()
    {
    }


    ImageImageTreeModel::CreateRootItemFunctor::CreateRootItemFunctor(
        AbstractTreeModel * tm, QThread * tt)
    {
      treeModel = tm;
      targetThread = tt;
      avgCharWidth = QFontMetrics(
          treeModel->getView()->getContentFont()).averageCharWidth();
    }


    ImageImageTreeModel::CreateRootItemFunctor::CreateRootItemFunctor(
        const CreateRootItemFunctor & other)
    {
      treeModel = other.treeModel;
      targetThread = other.targetThread;
      avgCharWidth = other.avgCharWidth;
    }


    ImageImageTreeModel::CreateRootItemFunctor::~CreateRootItemFunctor()
    {
      treeModel = NULL;
      targetThread = NULL;
    }


    ImageParentItem * ImageImageTreeModel::CreateRootItemFunctor::operator()(
        ControlCubeGraphNode * const & node) const
    {
      ImageParentItem * parentItem =
          new ImageParentItem(node, avgCharWidth);
      parentItem->setSelectable(false);
      parentItem->moveToThread(targetThread);

      QList< ControlCubeGraphNode * > connectedNodes = node->getAdjacentNodes();

      for (int j = 0; j < connectedNodes.size(); j++)
      {
        ControlCubeGraphNode * connectedNode = connectedNodes[j];
        ImageLeafItem * serialItem =
          new ImageLeafItem(connectedNode, avgCharWidth, parentItem);
        serialItem->setSelectable(false);
        serialItem->moveToThread(targetThread);

        parentItem->addChild(serialItem);
      }

      return parentItem;
    }


    void ImageImageTreeModel::CreateRootItemFunctor::addToRootItem(
        QAtomicPointer< RootItem > & root, ImageParentItem * const & item)
    {
      if (!root)
      {
        root = new RootItem;
        root->moveToThread(item->thread());
      }

      if (item)
        root->addChild(item);
    }


    ImageImageTreeModel::CreateRootItemFunctor &
        ImageImageTreeModel::CreateRootItemFunctor::operator=(
        const CreateRootItemFunctor & other)
    {
      if (this != &other)
      {
        treeModel = other.treeModel;
        avgCharWidth = other.avgCharWidth;
      }

      return *this;
    }


    void ImageImageTreeModel::rebuildItems()
    {
  //     cerr << "ImageImageTreeModel::rebuildItems called\n";
      if (!isFrozen())
      {
        emit cancelSort();
        setRebuilding(true);
        emit filterCountsChanged(-1, getTopLevelItemCount());
        QFuture< QAtomicPointer< RootItem > > futureRoot;

        if (getRebuildWatcher()->isStarted())
        {
          futureRoot = getRebuildWatcher()->future();
          futureRoot.cancel();
    //       futureRoot.waitForFinished();
    //       if (futureRoot.result())
    //         delete futureRoot.result();
        }

        futureRoot = QtConcurrent::mappedReduced(
            getControlNetwork()->GetCubeGraphNodes(),
            CreateRootItemFunctor(this, QThread::currentThread()),
            &CreateRootItemFunctor::addToRootItem,
            QtConcurrent::OrderedReduce | QtConcurrent::SequentialReduce);

        getRebuildWatcher()->setFuture(futureRoot);
      }
      else
      {
        queueRebuild();
      }
  //     cerr << "/ImageImageTreeModel::rebuildItems\n";
    }
  }
}
