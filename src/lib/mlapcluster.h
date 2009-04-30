#ifndef GUARD_mlapcluster_h
#define GUARD_mlapcluster_h

#include <ml.h>

#define CV_TYPE_NAME_ML_APCLUSTER	"opencv-ml-affinity-propagation-cluster"

struct CV_EXPORTS CvAPCParams
{
	int maxiteration;
	int stopcriterion;
	double lambda;

	CvAPCParams()
	: maxiteration(2000), stopcriterion(200), lambda(0.9)
	{}

	CvAPCParams( int _maxiteration, int _stopcriterion, double _lambda )
	: maxiteration(_maxiteration), stopcriterion(_stopcriterion), lambda(_lambda)
	{}
};

class CV_EXPORTS CvAPCluster : public CvStatModel
{
	private:
		CvAPCParams params;
	public:
		CvAPCluster( CvAPCParams _params );
		virtual ~CvAPCluster();
		virtual bool train( const CvMat* _train_data, const CvMat* _response );
		virtual bool train( const CvSparseMat* _train_data, const CvMat* _response );
		virtual void clear();
		virtual void write( CvFileStorage* fs, const char* name );
		virtual void read( CvFileStorage* fs, CvFileNode* root_node );
};

#endif
