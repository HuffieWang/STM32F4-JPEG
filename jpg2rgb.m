image = imread('E:\lenna.jpg');
imshow(image)
image = uint16(image);
temp = image(:,:,1)/2^3*2^11 + image(:,:,2)/2^3*2^6 + image(:,:,3)/2^3

