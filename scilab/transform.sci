exec('TraceTransform.sci');

function [sinogram, circus_function] = get_transform(padded ,tfunctionals, pfunctionals, angle_stepsize, orthonormal)
    [sinogram circus_function] = TraceTransform(padded, tfunctionals, pfunctionals, angle_stepsize, orthonormal);
endfunction

function [padded, base_name] = prepare_transform(input_orig, filename, angle_stepsize, orthonormal)
    if orthonormal
        // Resizing and padding the image with zeros
        ndiag = ceil(360/angle_stepsize);
        nrows = ceil(ndiag/sqrt(2));
        // Resizing the image to generate a square sinogram
        input_resized = imresize(im2uint8(input_orig), [nrows nrows]);
        M = nrows;
        N = nrows;
    else
        input_resized = im2uint8(input_orig);
        M = size(input_resized, 1);
        N = size(input_resized, 2);
    end
    origin = floor(([M N]+1)/2);

    temp1 = M - 1 - origin(1);
    temp2 = N - 1 - origin(2);
    rLast = ceil(sqrt((temp1*temp1+temp2*temp2))) + 1;
    rFirst = -rLast ;
    Nbins = int(rLast - rFirst + 1);

    // Padding the image
    padded = zeros(Nbins, Nbins);
    origin_padded = floor((size(padded)+1)/2);
    df = origin_padded - origin;
    padded(1+df(1):M+df(1),1+df(2):N+df(2)) = input_resized;
    padded = im2double(uint8(padded));

    // Find out the file basename
    [dummy, base_name, dummy] = fileparts(filename);
endfunction
