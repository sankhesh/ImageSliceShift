#include <vtkColorTransferFunction.h>
#include <vtkDICOMImageReader.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkImplicitPlaneRepresentation.h>
#include <vtkImplicitPlaneWidget2.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkImageActor.h>
#include <vtkImageReslice.h>
#include <vtkImageMapper3D.h>
#include <vtkPlaneSource.h>
#include <vtkMatrix4x4.h>
#include <vtkMath.h>
#include <vtkTransform.h>

//-----------------------------------------------------------------------------
#define print(name, length) \
  std::cout << #name << " : "; \
  for(int i = 0; i < length; ++i) \
    { \
    std::cout << name[i] << " "; \
    } \
  std::cout << std::endl; \

//-----------------------------------------------------------------------------
class vtkISSCallback : public vtkCommand
{
public:
  static vtkISSCallback * New()
    {
    return new vtkISSCallback;
    }

  virtual void Execute(vtkObject* caller, unsigned long, void*)
    {
    vtkImplicitPlaneWidget2 * planeWidget =
      reinterpret_cast<vtkImplicitPlaneWidget2*> (caller);
    vtkImplicitPlaneRepresentation * rep =
      reinterpret_cast<vtkImplicitPlaneRepresentation*>
      (planeWidget->GetRepresentation());
    rep->GetPlane(this->Plane);

    double * bounds = rep->GetBounds();

    // Slice
    double origin[3], normal[3];
    this->Plane->GetOrigin(origin);
    this->Plane->GetNormal(normal);

    double oldx[3], oldy[3], oldz[3];
    this->ImageReslice->GetResliceAxesDirectionCosines(oldx,oldy,oldz);
    double axis[3];
    vtkMath::Cross(oldz,normal,axis);
    if ( vtkMath::Normalize(axis) < 1.0e-15 )
      {
      this->ImageReslice->SetResliceAxesOrigin(origin);
      return;
      }
    double cos_theta = vtkMath::Dot(oldz, normal);
    double theta = vtkMath::DegreesFromRadians(acos( cos_theta ));
    vtkNew<vtkTransform> transform;
    transform->Identity();
    transform->Translate(origin[0],origin[1],origin[2]);
    transform->RotateWXYZ(theta,axis);
    transform->Translate(-origin[0],-origin[1],-origin[2]);

    //Set the new x
    double x[3];
    transform->TransformNormal(oldx,x);
    //Set the new y
    double y[3];
    transform->TransformNormal(oldy,y);

    this->ImageReslice->SetResliceAxesDirectionCosines(x,y,normal);
    this->ImageReslice->SetResliceAxesOrigin(origin);
    Ren->ResetCamera();
    }

  vtkISSCallback() : Plane(0), ImageReslice(0) {}

  vtkPlane* Plane;
  vtkImageReslice* ImageReslice;
  vtkRenderer* Ren;
};

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  vtkNew<vtkDICOMImageReader> reader;
  reader->SetDirectoryName(argv[1]);
  reader->Update();

  vtkImageData* im = reader->GetOutput();
  double range[2];
  im->GetScalarRange(range);
  int imDims[3], extent[6];
  double imSpacing[3], imOrigin[3], imBounds[6];
  im->GetDimensions(imDims);
  im->GetExtent(extent);
  im->GetSpacing(imSpacing);
  im->GetOrigin(imOrigin);
  im->GetBounds(imBounds);

  print (imDims, 3);
  print (imOrigin, 3);
  print (imSpacing, 3);
  print (imBounds, 6);

  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(range[0], 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(324.13, 1.0, 0.2, 0.2);
  ctf->AddRGBPoint(510.14, 1.0, 0.2, 0.2);
  ctf->AddRGBPoint(696.15, 0.9, 0.9, 0.9);
  ctf->AddRGBPoint(714.75, 1.0, 1.0, 1.0);
  ctf->AddRGBPoint(785.43, 1.0, 1.0, 1.0);
  ctf->AddRGBPoint(range[1], 1.0, 1.0, 1.0);

  vtkNew<vtkPiecewiseFunction> pf;
  pf->AddPoint(range[0], 0.0);
  pf->AddPoint(324.13, 0.0);
  pf->AddPoint(510.14, 0.5);
  pf->AddPoint(696.15, 0.5);
  pf->AddPoint(714.75, 0.7);
  pf->AddPoint(785.43, 1.0);
  pf->AddPoint(range[1], 1.0);

  vtkNew<vtkVolumeProperty> property;
  property->ShadeOn();
  property->SetColor(ctf.GetPointer());
  property->SetScalarOpacity(pf.GetPointer());

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper.GetPointer());
  volume->SetProperty(property.GetPointer());

  vtkNew<vtkRenderer> renderer;
  renderer->SetViewport(0, 0, 0.5, 1);
  renderer->AddVolume(volume.GetPointer());

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(800, 400);
  renderWindow->AddRenderer(renderer.GetPointer());

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow.GetPointer());

  vtkNew<vtkPlane> plane;

  vtkNew<vtkImplicitPlaneRepresentation> rep;
  rep->SetPlaceFactor(1.25); // This must be set prior to placing the widget
  rep->PlaceWidget(volume->GetBounds());
  rep->SetNormal(plane->GetNormal());
  //rep->SetOrigin(0,0,50); //this doesn't seem to work?

  vtkNew<vtkImplicitPlaneWidget2> planeWidget;
  planeWidget->SetInteractor(renderWindowInteractor.GetPointer());
  planeWidget->SetRepresentation(rep.GetPointer());

  // Setup the callback to do the work
  vtkNew<vtkImageReslice> reslice;
  double resliceAxes[16] =
    { 1.0, 0.0, 0.0, 0.0,
      0.0, 0.0,-1.0, 0.0,
      0.0, 1.0,-0.0, 0.0,
      //9.514, 112.464, 55.429,
      imBounds[0] + 0.5 * (imBounds[1] - imBounds[0]),
      imBounds[2] + 0.5 * (imBounds[3] - imBounds[2]),
      imBounds[4] + 0.5 * (imBounds[5] - imBounds[4]),
      1.0 };
  vtkNew<vtkMatrix4x4> resliceMatrix;
  resliceMatrix->DeepCopy(resliceAxes);
  resliceMatrix->Transpose();
  reslice->SetResliceAxes(resliceMatrix.GetPointer());
  reslice->SetInputConnection(reader->GetOutputPort());
  reslice->SetOutputDimensionality(2);
  reslice->SetOutputScalarType(VTK_UNSIGNED_CHAR);
  reslice->SetOutputSpacing(0.5, 0.5, 1);
//  reslice->SetOutputOrigin(bounds[0] + 0.5 * (bounds[1] - bounds[0]),
//                           bounds[2] + 0.5 * (bounds[3] - bounds[2]),
//                           bounds[4] + 0.5 * (bounds[5] - bounds[4]));

  reslice->Update();
  vtkImageData* sliceData = reslice->GetOutput();
  double sliceBounds[6];
  sliceData->GetBounds(sliceBounds);
  int sliceExtent[6];
  sliceData->GetExtent(sliceExtent);
  print(sliceBounds, 6);
  print (sliceExtent, 6);

  vtkNew<vtkISSCallback> callback;
  callback->Plane = plane.GetPointer();
  callback->ImageReslice = reslice.GetPointer();
  planeWidget->AddObserver(vtkCommand::InteractionEvent, callback.GetPointer());

  // Display the image
  vtkNew<vtkImageActor> actor;
  actor->GetMapper()->SetInputConnection(reslice->GetOutputPort());
  //renderer->AddActor(actor.GetPointer());

  vtkNew<vtkRenderer> sliceRen;
  sliceRen->SetViewport(0.5, 0, 1, 1);
  sliceRen->SetBackground(0.1, 0.1, 0.0);
  sliceRen->AddActor(actor.GetPointer());
  renderWindow->AddRenderer(sliceRen.GetPointer());
  callback->Ren = sliceRen.GetPointer();

  // Enable the widget
  sliceRen->ResetCamera();
  renderer->ResetCamera();
  renderWindowInteractor->Initialize();
  planeWidget->On();
  renderWindow->Render();

  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
