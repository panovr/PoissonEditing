/*=========================================================================
 *
 *  Copyright David Doria 2012 daviddoria@gmail.com
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "PoissonEditing.h"

// STL
#include <iostream>

// ITK
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkCastImageFilter.h"
#include "itkNeighborhoodOperatorImageFilter.h"
#include "itkBackwardDifferenceOperator.h"

typedef itk::Image<float, 2> FloatScalarImageType;

void DerivativesToLaplacian(FloatScalarImageType::Pointer xDerivative,
                            FloatScalarImageType::Pointer yDerivative,
                            FloatScalarImageType::Pointer laplacian);

int main(int argc, char* argv[])
{
  // Verify arguments
  if(argc < 4)
    {
    std::cout << "Usage: inputImage maskImage derivativeImage outputImage" << std::endl;
    exit(-1);
    }

  // Parse arguments
  std::string sourceImageFilename = argv[1];
  std::string maskFilename = argv[2];
  std::string derivativeImageFilename = argv[3];
  std::string outputFilename = argv[4];

  // Output arguments
  std::cout << "Input " << sourceImageFilename << std::endl
            << "Mask " << maskFilename << std::endl
            << "derivativeImage " << derivativeImageFilename << std::endl
            << "output " << outputFilename << std::endl;

  // Read files
  typedef itk::ImageFileReader<FloatScalarImageType> ImageReaderType;
  ImageReaderType::Pointer sourceImageReader = ImageReaderType::New();
  sourceImageReader->SetFileName(sourceImageFilename);
  sourceImageReader->Update();

  //typedef itk::Image<itk::CovariantVector<float, 2>, 2> DerivativeImageType;
  typedef itk::VectorImage<float, 2> DerivativeImageType;
  typedef itk::ImageFileReader<DerivativeImageType> DerivativeImageReaderType;
  DerivativeImageReaderType::Pointer derivativeImageReader = DerivativeImageReaderType::New();
  derivativeImageReader->SetFileName(derivativeImageFilename);
  derivativeImageReader->Update();

  typedef itk::Image<unsigned char, 2> UnsignedCharScalarImageType;

  Mask::Pointer mask = Mask::New();
  mask->Read(maskFilename);

  // Extract components of derivative
  FloatScalarImageType::Pointer derivativeX = FloatScalarImageType::New();
  ITKHelpers::ExtractChannel(derivativeImageReader->GetOutput(), 0, derivativeX.GetPointer());
  ITKHelpers::WriteImage(derivativeX.GetPointer(), "xDerivative.mhd");

  FloatScalarImageType::Pointer derivativeY = FloatScalarImageType::New();
  ITKHelpers::ExtractChannel(derivativeImageReader->GetOutput(), 1, derivativeY.GetPointer());
  ITKHelpers::WriteImage(derivativeY.GetPointer(), "yDerivative.mhd");

  // Create Laplacian from derivatives
  FloatScalarImageType::Pointer laplacian = FloatScalarImageType::New();
  DerivativesToLaplacian(derivativeX, derivativeY, laplacian);
  ITKHelpers::WriteImage(laplacian.GetPointer(), "LaplacianFromDerivatives.mhd");

  // Fill hole
  PoissonEditing<float> poissonEditing;
  poissonEditing.SetTargetImage(sourceImageReader->GetOutput());
  poissonEditing.SetLaplacian(laplacian.GetPointer());
  poissonEditing.SetMask(mask);
  poissonEditing.FillMaskedRegion();

  FloatScalarImageType::Pointer outputImage = poissonEditing.GetOutput();

  ITKHelpers::WriteImage(outputImage.GetPointer(), outputFilename);

  return EXIT_SUCCESS;
}

void DerivativesToLaplacian(FloatScalarImageType::Pointer xDerivative,
                            FloatScalarImageType::Pointer yDerivative,
                            FloatScalarImageType::Pointer laplacian)
{
  // Laplacian of a scalar field = divergence of the gradient of the scalar field
  // Take the derivative of the x derivative and add it to the derivative of the y derivative
  laplacian->SetRegions(xDerivative->GetLargestPossibleRegion());
  laplacian->Allocate();

  // Take derivatives of both derivative images
#if 0
  typedef itk::DerivativeImageFilter<FloatScalarImageType, FloatScalarImageType>  DerivativeFilterType;

  DerivativeFilterType::Pointer derivativeFilterX = DerivativeFilterType::New();
  derivativeFilterX->SetInput(xDerivative);
  derivativeFilterX->SetDirection(0); // "x" axis
  derivativeFilterX->Update();

  DerivativeFilterType::Pointer derivativeFilterY = DerivativeFilterType::New();
  derivativeFilterY->SetInput(yDerivative);
  derivativeFilterY->SetDirection(1); // "y" axis
  derivativeFilterY->Update();
#endif

  typedef itk::BackwardDifferenceOperator<float, 2> OperatorType;
  itk::Size<2> radius;
  radius.Fill(1); // a radius of 1x1 creates a 3x3 operator

  OperatorType operatorX;
  operatorX.SetDirection(0); // Create the operator for the X axis derivative
  operatorX.CreateToRadius(radius);

  OperatorType operatorY;
  operatorY.SetDirection(1); // Create the operator for the Y axis derivative
  operatorY.CreateToRadius(radius);

  typedef itk::NeighborhoodOperatorImageFilter<FloatScalarImageType, FloatScalarImageType> NeighborhoodOperatorImageFilterType;

  NeighborhoodOperatorImageFilterType::Pointer filterX = NeighborhoodOperatorImageFilterType::New();
  filterX->SetOperator(operatorX);
  filterX->SetInput(xDerivative);
  filterX->Update();

  NeighborhoodOperatorImageFilterType::Pointer filterY = NeighborhoodOperatorImageFilterType::New();
  filterY->SetOperator(operatorY);
  filterY->SetInput(yDerivative);
  filterY->Update();

  itk::ImageRegion<2> region = xDerivative->GetLargestPossibleRegion();

  itk::ImageRegionConstIterator<FloatScalarImageType> xSecondDerivativeIterator(filterX->GetOutput(), region);
  itk::ImageRegionConstIterator<FloatScalarImageType> ySecondDerivativeIterator(filterY->GetOutput(), region);
  itk::ImageRegionIterator<FloatScalarImageType> laplacianIterator(laplacian, region);

  while(!laplacianIterator.IsAtEnd())
    {
    //laplacianIterator.Set(2.* xSecondDerivativeIterator.Get() + 2.* ySecondDerivativeIterator.Get());
    laplacianIterator.Set(xSecondDerivativeIterator.Get() + ySecondDerivativeIterator.Get());

    ++xSecondDerivativeIterator;
    ++ySecondDerivativeIterator;
    ++laplacianIterator;
    }

}
