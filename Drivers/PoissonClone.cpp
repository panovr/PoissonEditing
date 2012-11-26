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
#include "itkVectorIndexSelectionCastImageFilter.h"
#include "itkComposeImageFilter.h"

int main(int argc, char* argv[])
{
  // Verify arguments
  if(argc < 6)
    {
    std::cout << "Usage: ImageToFill mask sourceImage guidanceField outputImage" << std::endl;
    return EXIT_FAILURE;
    }

  // Parse arguments
  std::string targetImageFilename = argv[1];
  std::string maskFilename = argv[2];
  std::string sourceImageFilename = argv[3];
  std::string guidanceFieldFilename = argv[4];
  std::string outputFilename = argv[5];

  // Output arguments
  std::cout << "Target image: " << targetImageFilename << std::endl
            << "Source image: " << sourceImageFilename << std::endl
            << "Mask image: " << maskFilename << std::endl
            << "Guidance field: " << guidanceFieldFilename << std::endl
            << "Output image: " << outputFilename << std::endl;

  typedef itk::VectorImage<float, 2> FloatVectorImageType;

  FloatVectorImageType::Pointer sourceImage = NULL;
  FloatVectorImageType::Pointer guidanceField = NULL;

  // Read images
  typedef itk::ImageFileReader<FloatVectorImageType> ImageReaderType;
  ImageReaderType::Pointer targetImageReader = ImageReaderType::New();
  targetImageReader->SetFileName(targetImageFilename);
  targetImageReader->Update();

  // Read mask
  Mask::Pointer mask = Mask::New();
  mask->Read(maskFilename);

  if(sourceImageFilename != "0")
  {
    ImageReaderType::Pointer sourceImageReader = ImageReaderType::New();
    sourceImageReader->SetFileName(sourceImageFilename);
    sourceImageReader->Update();

    sourceImage = sourceImageReader->GetOutput();
  }

  if(guidanceFieldFilename != "0")
  {
    ImageReaderType::Pointer guidanceFieldReader = ImageReaderType::New();
    guidanceFieldReader->SetFileName(sourceImageFilename);
    guidanceFieldReader->Update();

    guidanceField = guidanceFieldReader->GetOutput();
  }
//   // Output image properties
//   std::cout << "Source image: " << sourceImageReader->GetOutput()->GetLargestPossibleRegion().GetSize() << std::endl
//             << "Target image: " << targetImageReader->GetOutput()->GetLargestPossibleRegion().GetSize() << std::endl
//             << "Mask image: " << mask->GetLargestPossibleRegion().GetSize() << std::endl;

  FloatVectorImageType::Pointer output = FloatVectorImageType::New();

  // TODO: Convert this to the new API
//   PoissonEditing<float>::FillAllChannels(sourceImage, targetImageReader->GetOutput(),
//                    mask, output.GetPointer());

  // Write output
  //Helpers::WriteImage<FloatVectorImageType>(reassembler->GetOutput(), "output.mhd");
  ITKHelpers::WriteRGBImage(output.GetPointer(), outputFilename);

  return EXIT_SUCCESS;
}