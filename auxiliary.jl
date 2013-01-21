
datawrite(file::String, data::Matrix) = datawrite(file, data, [])
function datawrite(file::String, data::Matrix, headers::Vector)
        @assert length(headers) == 0 || length(headers) == size(data, 2)

        # Calculate column width
        rows = size(data, 1)
        cols = size(data, 2)
        widths::Vector = zeros(Uint, cols)
        for col in 1:cols
                if length(headers) > 0
                        widths[col] = length(headers[col])
                end
                for row in 1:rows
                        widths[col] = max(widths[col], length(string(data[row, col])))
                end
                widths[col] += 2        # add spacing
        end

        # Open file
        s = open(file, "w")

        # Print headers
        if length(headers) > 0
                write(s, "%  ");
                for col in 1:cols
                        write(s, headers[col])
                        write(s, " "^(widths[col]-length(headers[col])))
                end
                write(s, "\n")
        end

        # Print data
        for row in 1:rows
                write(s, "   ");
                for col in 1:cols
                        write(s, string(data[row, col]))
                        write(s, " "^(widths[col]-length(string(data[row, col]))))
                end
                write(s, "\n")
        end

        close(s)
end