#pragma once

#include <GL/glew.h>
// Include this first to avoid winsock2.h problems on Windows:
#include <maya/MTypes.h>
// Maya API includes
#include <maya/MDagPath.h>
#include <maya/MFnPlugin.h>
#include <maya/MFnMesh.h>
#include <maya/MFloatPointArray.h>
#include <maya/MIntArray.h>
#include <maya/MUintArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>

// Viewport 2.0 includes
#include <maya/MDrawRegistry.h>
#include <maya/MPxDrawOverride.h>
#include <maya/MUserData.h>
#include <maya/MDrawContext.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MPxGeometryOverride.h>

//---------------------------------------------------------------------------
#include <maya/MPointArray.h>
#include <maya/MFnCamera.h>
#include <iostream>
#include  <vector>
#include <map>
using namespace std;
/*
// 
// class vixo_fishEyePreviewGeometryOverride : public MHWRender::MPxGeometryOverride
// {
// public:
// 	static MPxGeometryOverride* Creator(const MObject& obj)
// 	{
// 		return new vixo_fishEyePreviewGeometryOverride(obj);
// 	}
// 
// 	virtual ~vixo_fishEyePreviewGeometryOverride()
// 	{
// 
// 	}
// 
// 	virtual MHWRender::DrawAPI supportedDrawAPIs() const
// 	{
// 		return MHWRender::kAllDevices;
// 	}
// 
// 	virtual void updateDG()
// 	{
// 		fnShape.syncObject();
// 	}
// 	virtual void updateRenderItems(
// 		const MDagPath& path,
// 		MHWRender::MRenderItemList& list);
// 	virtual void populateGeometry(
// 		const MHWRender::MGeometryRequirements& requirements,
// 		MHWRender::MRenderItemList& renderItems,
// 		MHWRender::MGeometry& data);
// 	virtual void cleanUp()
// 	{
// 
// 	}
// 	MFnMesh fnShape;
// protected:
// 	vixo_fishEyePreviewGeometryOverride(const MObject& obj)
// 		: MPxGeometryOverride(obj)
// 	{
// 		if(!fnShape.hasObj(MFn::kMesh))
// 			fnShape.setObject(obj);
// 		fnShape.syncObject();
// 	}
// };
*/

class vixo_fishEyePreviewData : public MUserData
{
public:
	vector<GLfloat> vertexArr,normalArr;
	vector<GLint> elementIndexArr;
	MColor color;
public:
	vixo_fishEyePreviewData() : MUserData(false) {}
	vixo_fishEyePreviewData(const MDagPath& objPath,const MDagPath& cameraPath)
	: MUserData(false) 
	{
		cout<<objPath.fullPathName().asChar()<<endl;
		MFnMesh fnMesh(objPath);
		MIntArray triCount,triConn;
		fnMesh.getTriangles(triCount,triConn);
		elementIndexArr.clear();
		elementIndexArr.resize(triConn.length());
		for(int i=0;i<triConn.length();i++)
		{
			elementIndexArr[i]=triConn[i];
		}
		vertexArr.clear();
		vertexArr.resize(fnMesh.numVertices()*3);
		normalArr.clear();
		normalArr.resize(fnMesh.numVertices()*3);

		MStringArray shadingEngines;
		MGlobal::executeCommand("listConnections -s false -d true -type \"shadingEngine\" "+objPath.fullPathName(),shadingEngines);
		if(shadingEngines.length()<=0)
		{
			color=MColor(0.5f,0.5f,0.5f);
			return;
		}
		cout<<shadingEngines[0].asChar()<<endl;
		MStringArray lamberts;
		MGlobal::executeCommand("listConnections -s true -d false -type \"lambert\" "+shadingEngines[0]+".surfaceShader",lamberts);
		if(lamberts.length()<=0)
		{
			color=MColor(0.5f,0.5f,0.5f);
			return;
		}
		cout<<lamberts[0].asChar()<<endl;
		MDoubleArray colorValue;
		MGlobal::executeCommand("getAttr "+lamberts[0]+".color",colorValue);
		color=MColor(colorValue[0],colorValue[1],colorValue[2]);
		cout<<colorValue[0]<<" "<<colorValue[1]<<" "<<colorValue[2]<<endl;
	}
	virtual ~vixo_fishEyePreviewData()
	{

	}
	void update(const MDagPath& objPath,const MDagPath& cameraPath)
	{
		MFnMesh fnMesh(objPath);
		MFnCamera fnCam(cameraPath);
		MVector upDir=fnCam.upDirection(MSpace::kWorld);
		MVector rightDir=fnCam.rightDirection(MSpace::kWorld);
		MVector viewDir=fnCam.viewDirection(MSpace::kWorld);
		MPoint eyePoint=fnCam.eyePoint(MSpace::kWorld);
		double angleOfView=fnCam.horizontalFieldOfView();
		//cout<<(angleOfView*180/M_PI)<<endl;
		MPointArray allPoints;
		fnMesh.getPoints(allPoints,MSpace::kWorld);

		for(int i=0;i<allPoints.length();i++)
		{
			MVector origRay=allPoints[i]-eyePoint;
			MVector origRayShadow=origRay-upDir.normal()*(upDir*origRay/upDir.length());//shadow on Plane
			MVector origRayShadowUpdir=upDir.normal()*(upDir*origRay/upDir.length());//shadow on updir
			//cout<<origRayShadow.length()<<" "<<origRayShadowUpdir.length()<<endl;
			double currentHeight=origRayShadowUpdir.length()/0.3375*(tan(angleOfView/2)/4.8);
			if(origRayShadowUpdir*upDir<0)
				currentHeight=-currentHeight;

			MVector vectorY=upDir.normal()*currentHeight;
			//cout<<currentHeight<<" "<<(8*tan(verticalAngleOfView/2))<<" "<<vectorY.y<<endl;

			double angle=viewDir.angle(origRayShadow);
			//angle=min(100.0,angle);
			if(cos(rightDir.angle(origRayShadow))<0)
				angle=-angle;

			double rotateAngle=asin(angle/M_PI*2*sin(angleOfView/2));
			double rotateAngleSin=angle/M_PI*2*sin(angleOfView/2);
			double rotateAngleCos=cos(rotateAngle);
			double rotateAngleTan=tan(rotateAngle);
			MVector currentRayPlane=viewDir.normal()*origRayShadow.length()+rightDir*(viewDir.normal()*origRayShadow.length()).length()*rotateAngleTan;
			//MVector currentRayPlane=viewDir.normal()*(origRayShadow.length()*cos(angleOfView/2))+rightDir.normal()*(origRayShadow.length()*sin(angleOfView/2)*rotateAngleSin);
			//MVector currentRayPlane=(viewDir.normal()*rotateAngleCos+rightDir.normal()*rotateAngleSin).normal()*origRayShadow.length();
			//cout<<currentRay.length()<<" "<<eyePoint.x<<" "<<eyePoint.y<<" "<<eyePoint.z<<" "<<vectorY.y<<endl;
			//MVector currentRay=(currentRayPlane+vectorY).normal()*origRayShadow.length();
			//cout<<currentRayPlane.y<<" "<<vectorY.y<<endl;
			allPoints[i]=eyePoint+currentRayPlane+vectorY;
			//cout<<allPoints[i].x<<" "<<allPoints[i].y<<" "<<allPoints[i].z<<endl;
			vertexArr[3*i+0]=allPoints[i].x;
			vertexArr[3*i+1]=allPoints[i].y;
			vertexArr[3*i+2]=allPoints[i].z;
		}
		MFloatVectorArray normals;
		fnMesh.getNormals(normals,MSpace::kWorld);
		for(int i=0;i<normals.length();i++)
		{
			normalArr[3*i+0]=normals[i].x;
			normalArr[3*i+1]=normals[i].y;
			normalArr[3*i+2]=normals[i].z;
		}
		//fnMesh.setPoints(allPoints,MSpace::kWorld);
		//fnMesh.updateSurface();
	}
};

//---------------------------------------------------------------------------

class vixo_fishEyePreviewDrawOverride : public MHWRender::MPxDrawOverride
{
public:
	static MHWRender::MPxDrawOverride* Creator(const MObject& obj) {
		
		return new vixo_fishEyePreviewDrawOverride(obj);
	}

	virtual ~vixo_fishEyePreviewDrawOverride(){}

	virtual MBoundingBox boundingBox(
		const MDagPath& objPath,
		const MDagPath& cameraPath) const
	{ 
		MPoint corner1( -10.0, -10.0, -10.0 );
		MPoint corner2( 10.0, 10.0, 10.0);

		return MBoundingBox(corner1, corner2);
	}

	virtual MUserData* prepareForDraw(
		const MDagPath& objPath,
		const MDagPath& cameraPath,
		MUserData* oldData)
	{
		cout<<"prepare"<<endl;
		vixo_fishEyePreviewData* data = dynamic_cast<vixo_fishEyePreviewData*>(oldData);
		if (!data)
		{
			// data did not exist or was incorrect type, create new
			data = new vixo_fishEyePreviewData(objPath,cameraPath);
		}
		data->update(objPath,cameraPath);
		return data;
	}

	static void draw(const MHWRender::MDrawContext& context, const MUserData* data)
	{
		cout<<"draw"<<endl;
		 vixo_fishEyePreviewData* mesh = const_cast<vixo_fishEyePreviewData*>(dynamic_cast<const vixo_fishEyePreviewData*>(data));
		// context.
		//static const float colorData[] = {1.0f, 0.0f, 0.0f};

		// set world matrix
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		MMatrix transform = context.getMatrix(MHWRender::MDrawContext::kWorldViewMtx);
		// set projection matrix
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		MMatrix projection = context.getMatrix(MHWRender::MDrawContext::kProjectionMtx);

		const int displayStyle = context.getDisplayStyle();
		glPushAttrib( GL_CURRENT_BIT );
		glPushAttrib( GL_ENABLE_BIT);
		bool isColorMaterial=glIsEnabled(GL_COLOR_MATERIAL);

		if(displayStyle & MHWRender::MDrawContext::kGouraudShaded) {
			if(!isColorMaterial)
			{

				glColorMaterial(GL_FRONT_AND_BACK,GL_DIFFUSE);
				glEnable(GL_COLOR_MATERIAL);

			}
			glColor3f(mesh->color.r,mesh->color.g,mesh->color.b);

			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}else if(displayStyle & MHWRender::MDrawContext::kWireFrame){
			glDisable(GL_LIGHTING);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}

		//glBindBuffer(GL_ARRAY_BUFFER, &mesh->vertexArr[0]);
		glVertexPointer(3, GL_FLOAT, 0, &mesh->vertexArr[0]);
		glEnableClientState(GL_VERTEX_ARRAY);
		glNormalPointer(GL_FLOAT, 0, &mesh->normalArr[0]);
		glEnableClientState(GL_NORMAL_ARRAY);

		glDrawElements(GL_TRIANGLES, mesh->elementIndexArr.size(), GL_UNSIGNED_INT, &mesh->elementIndexArr[0]);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
		//glBindBuffer(GL_ARRAY_BUFFER, 0);
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		if(!isColorMaterial)
			glDisable(GL_COLOR_MATERIAL);
		glPopAttrib();
		glPopAttrib();

		glPopMatrix();

		//glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		//glColor3f(1, 1, 1);
	}

private:
	vixo_fishEyePreviewDrawOverride(const MObject& obj)
		: MHWRender::MPxDrawOverride(obj, vixo_fishEyePreviewDrawOverride::draw)
	{

	}
	MString animRealCamera;
	double resolutionAspect;
//	bool getSelectionStatus(const MDagPath& objPath) const;
public:
/*
// 	static void setAnimCamera(MString animRealCam)
// 	{
// 		animRealCamera=animRealCam;
// 	}
// 	static void setResolution(double resolutionAsp)
// 	{
// 		resolutionAspect=resolutionAsp;
// 	}
*/
};

/*
---------------------------------------------------------------------------
Control command

class vixo_fishEyePreviewCommand : public MPxCommand
{
public:
	virtual MStatus doIt(const MArgList &args);
	static void *Creator();
};

void*
	vixo_fishEyePreviewCommand::Creator()
{
	return new vixo_fishEyePreviewCommand();
}

MStatus vixo_fishEyePreviewCommand::doIt(const MArgList &args)
{
	MSyntax syntax;
	syntax.addFlag("c", "camera", MSyntax::kString);
	syntax.addFlag("a", "aspect", MSyntax::kDouble);
	MArgDatabase argDB(syntax, args);

	if(argDB.isFlagSet("c"))
	{
		MString camera;
		argDB.getFlagArgument("c", 0, camera);
		vixo_fishEyePreviewDrawOverride::setAnimCamera(camera);
	}

	if(argDB.isFlagSet("a"))
	{
		double aspect;
		argDB.getFlagArgument("a", 0, aspect);
		vixo_fishEyePreviewDrawOverride::setResolution(aspect);
	}

	return MS::kSuccess;
}*/
MString drawDbClassification("drawdb/geometry/mesh");
MString drawRegistrantId("OpenSubdivDrawOverridePlugin");
MStatus initializePlugin( MObject obj )
{
#if defined(_WIN32) && defined(_DEBUG)
	// Disable buffering for stdout and stderr when using debug versions
	// of the C run-time library.
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
#endif

	MStatus   status;
	MFnPlugin plugin( obj, PLUGIN_COMPANY, "3.0", "Any");

	status = MHWRender::MDrawRegistry::registerDrawOverrideCreator(
		drawDbClassification,
		drawRegistrantId,
		vixo_fishEyePreviewDrawOverride::Creator);
	if (!status) {
		status.perror("registerDrawOverrideCreator");
		return status;
	}
	glewInit();

	return status;
}

MStatus uninitializePlugin( MObject obj)
{
	MStatus   status;
	MFnPlugin plugin( obj );

	status = MHWRender::MDrawRegistry::deregisterDrawOverrideCreator(
		drawDbClassification,
		drawRegistrantId);
	if (!status) {
		status.perror("deregisterDrawOverrideCreator");
		return status;
	}

	return status;
}

