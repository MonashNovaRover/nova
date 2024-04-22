/*
 * Functions for chart analysis
 * Author: Connor Macdougall
*/

export const defaultPeakFinder = (data: number[][]): number[][] | undefined => {

    let peaks: number[][] = [];
    const RESOLUTION = 2;           // The amount of points to each side of a peak that must show a downwards trend away from the peak

    const peak_percentage_diff = 20;                        // e.g. 20 means peak must be 20% greater than values at either end of resolution range.
    const SCALE_FACTOR = 1 + peak_percentage_diff/100;      // The scale factor that the peak must be greater than both ends of the resolution of the peak 
                                                            // (e.g. for scale_factor of 1.4: a peak with 50 of 'RESOLUTION' units to the right of it, and 60 'RESOLUTION' units to the left of it must be > 1.4*max(50,60) = 84)
    
    if (data.length > 0) {
        for (let middleIndex = 0; middleIndex < data.length; middleIndex++) {
            let previous: boolean = true
            for (let range = middleIndex - RESOLUTION; range <= middleIndex + RESOLUTION; range++) {
                if (!previous) {
                    break
                }
                if (range >= 0 && range < data.length) {
                    if (range == middleIndex) {
                        previous = true
                    } else if (range > middleIndex) {
                        if ((range == data.length - 1 && middleIndex + RESOLUTION > data.length - 1) || range == middleIndex + RESOLUTION) {    // Checking for scale factor
                            if (data[middleIndex][1] > SCALE_FACTOR*data[range][1]) {
                                previous = true
                            } else {
                                previous = false
                            }
                        } else if (range == data.length - 1 || data[range - 1][1] > data[range][1]) {                                           // Checking downwards gradient away from peak
                            previous = true
                        } else {
                            previous = false
                        }
                    } else {
                        if ((range == 0 && middleIndex - RESOLUTION < 0) || range == middleIndex - RESOLUTION) {                                // Checking for scale factor
                            if (data[middleIndex][1] > SCALE_FACTOR*data[range][1]) {
                                previous = true
                            } else {
                                previous = false
                            }
                        } else if (range == 0 || data[range][1] < data[range + 1][1]) {                                                         // Checking downwards gradient away from peak
                            previous = true
                        } else {
                            previous = false
                        }
                    }
                } else {
                    previous = true
                }
            }
            if (previous) {
                peaks.push(data[middleIndex]);
            }
        }
    }
    return peaks;
}