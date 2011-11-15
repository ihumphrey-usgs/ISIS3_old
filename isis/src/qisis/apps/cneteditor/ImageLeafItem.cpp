#include "IsisDebug.h"

#include "ImageLeafItem.h"


namespace Isis
{
  namespace CnetViz
  {
    ImageLeafItem::ImageLeafItem(ControlCubeGraphNode * node,
        int avgCharWidth, AbstractTreeItem * parent)
        : AbstractTreeItem(parent), AbstractImageItem(node, avgCharWidth)
    {
    }


    ImageLeafItem::~ImageLeafItem()
    {
    }
  }
}
