
__kernel void image_kernel(
        __read_only image2d_t leftImage,
        __read_only image2d_t rightImage,
        __write_only image2d_t outImage
) {
    int2 pos = {get_global_id(0), get_global_id(1)};


    // Read left image from (pos.x, pos.y)
    int sum = 0;
    int kernelSize = 49;
    int minAverage = 255;
    int pixelDisparity = 0;
    int disparityLevel = 105;
    int2 readPosLeft, readPosRight;

    //if (pos.x > 101 && pos.y > 30 && pos.x < 200 && pos.y < 230) {
    //if (pos.x == 101 && pos.y == 101) {
    if (pos.y > 0 && pos.x > disparityLevel && pos.y < 549 && pos.x < 639) {

        // Disparity level max
        for (int d = 0; d < disparityLevel; ++d) {
            // Kernel find absolute differences along x axis
            for (int row = -3; row < 4; ++row) {
                for (int col = -3; col < 4; ++col) {
                    readPosLeft.xy = (int2)(pos.x + col, pos.y + row);
                    readPosRight.xy =  (int2)(pos.x + col - d, pos.y + row);
                    //printf("leftX: %i, leftY: %i, rightX: %i, rightY: %i\n", readPosLeft.x, readPosLeft.y, readPosRight.x , readPosRight.y);

                    //printf("Col: %i, Row: %i\n", col, row);
                    int4 imgLeft = read_imagei(leftImage, readPosLeft);
                    int4 imgRight = read_imagei(rightImage, readPosRight);
                    sum += abs(imgLeft.x - imgRight.x);
                }
            }


            int average = sum / kernelSize;

            // If this kernel average was lower than the previous average then keep it
            if (average < minAverage) {
                minAverage = average;
                pixelDisparity = d;
            }
            //printf("Average: %i | d: %i\n", average, d);

            sum = 0;
        }
        write_imagei(outImage, pos, pixelDisparity);
        pixelDisparity = 0;
        minAverage = 100; // Reset min for avg
    }
    else {
        write_imagei(outImage, pos, 0);

    }


}
