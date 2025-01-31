// PngImage.hpp must be included first because of png issues on Ubuntu 16.04.
#include "PngImage.hpp"

// Shasta.
#include "Assembler.hpp"
#include "AlignmentGraph.hpp"
#include "timestamp.hpp"
using namespace shasta;

// Standard libraries.
#include "chrono.hpp"
#include "iterator.hpp"
#include "tuple.hpp"



// Compute a marker alignment of two oriented reads.
void Assembler::alignOrientedReads(
    ReadId readId0, Strand strand0,
    ReadId readId1, Strand strand1,
    size_t maxSkip,     // Maximum ordinal skip allowed.
    size_t maxDrift,    // Maximum ordinal drift allowed.
    uint32_t maxMarkerFrequency
)
{
    alignOrientedReads(
        OrientedReadId(readId0, strand0),
        OrientedReadId(readId1, strand1),
        maxSkip, maxDrift, maxMarkerFrequency
        );
}
void Assembler::alignOrientedReads(
    OrientedReadId orientedReadId0,
    OrientedReadId orientedReadId1,
    size_t maxSkip, // Maximum ordinal skip allowed.
    size_t maxDrift,    // Maximum ordinal drift allowed.
    uint32_t maxMarkerFrequency
)
{
    checkReadsAreOpen();
    checkReadNamesAreOpen();
    checkMarkersAreOpen();


    // Get the markers sorted by kmerId.
    array<vector<MarkerWithOrdinal>, 2> markersSortedByKmerId;
    vector<MarkerWithOrdinal> markers1SortedByKmerId;
    getMarkersSortedByKmerId(orientedReadId0, markersSortedByKmerId[0]);
    getMarkersSortedByKmerId(orientedReadId1, markersSortedByKmerId[1]);

    // Call the lower level function.
    AlignmentGraph graph;
    Alignment alignment;
    AlignmentInfo alignmentInfo;
    const bool debug = true;
    alignOrientedReads(
        markersSortedByKmerId,
        maxSkip, maxDrift, maxMarkerFrequency, debug, graph, alignment, alignmentInfo);

    // Compute the AlignmentInfo.
    uint32_t leftTrim;
    uint32_t rightTrim;
    tie(leftTrim, rightTrim) = alignmentInfo.computeTrim();
    cout << orientedReadId0 << " has " << reads[orientedReadId0.getReadId()].baseCount;
    cout << " bases and " << markersSortedByKmerId[0].size() << " markers." << endl;
    cout << orientedReadId1 << " has " << reads[orientedReadId1.getReadId()].baseCount;
    cout << " bases and " << markersSortedByKmerId[1].size() << " markers." << endl;
    cout << "The alignment has " << alignmentInfo.markerCount;
    cout << " markers. Left trim " << leftTrim;
    cout << " markers, right trim " << rightTrim << " markers." << endl;

#if 0
    // For convenience, also write the two oriented reads.
    ofstream fasta("AlignedOrientedReads.fasta");
    writeOrientedRead(orientedReadId0, fasta);
    writeOrientedRead(orientedReadId1, fasta);
#endif
}



// This lower level version takes as input vectors of
// markers already sorted by kmerId.
void Assembler::alignOrientedReads(
    const array<vector<MarkerWithOrdinal>, 2>& markersSortedByKmerId,
    size_t maxSkip,             // Maximum ordinal skip allowed.
    size_t maxDrift,            // Maximum ordinal drift allowed.
    uint32_t maxMarkerFrequency
)
{
    // Compute the alignment.
    AlignmentGraph graph;
    Alignment alignment;
    AlignmentInfo alignmentInfo;
    const bool debug = true;
    alignOrientedReads(
        markersSortedByKmerId,
        maxSkip, maxDrift, maxMarkerFrequency, debug, graph, alignment, alignmentInfo);
}



void Assembler::alignOrientedReads(
    const array<vector<MarkerWithOrdinal>, 2>& markersSortedByKmerId,
    size_t maxSkip,             // Maximum ordinal skip allowed.
    size_t maxDrift,            // Maximum ordinal drift allowed.
    uint32_t maxMarkerFrequency,
    bool debug,
    AlignmentGraph& graph,
    Alignment& alignment,
    AlignmentInfo& alignmentInfo
)
{
    align(markersSortedByKmerId,
        maxSkip, maxDrift, maxMarkerFrequency, debug, graph, alignment, alignmentInfo);
}



// Compute marker alignments of an oriented read with all reads
// for which we have an alignment.
void Assembler::alignOverlappingOrientedReads(
    ReadId readId, Strand strand,
    size_t maxSkip,                 // Maximum ordinal skip allowed.
    size_t maxDrift,                // Maximum ordinal drift allowed.
    uint32_t maxMarkerFrequency,
    size_t minAlignedMarkerCount,   // Minimum number of markers in an alignment.
    size_t maxTrim                  // Maximum trim (number of markers) allowed in an alignment.
)
{
    alignOverlappingOrientedReads(
        OrientedReadId(readId, strand),
        maxSkip, maxDrift, maxMarkerFrequency, minAlignedMarkerCount, maxTrim);
}



void Assembler::alignOverlappingOrientedReads(
    OrientedReadId orientedReadId0,
    size_t maxSkip,                 // Maximum ordinal skip allowed.
    size_t maxDrift,                // Maximum ordinal drift allowed.
    uint32_t maxMarkerFrequency,
    size_t minAlignedMarkerCount,   // Minimum number of markers in an alignment.
    size_t maxTrim                  // Maximum trim (number of markers) allowed in an alignment.
)
{
    // Check that we have what we need.
    checkReadsAreOpen();
    checkMarkersAreOpen();
    checkAlignmentCandidatesAreOpen();

    // Get the markers for orientedReadId0.
    array<vector<MarkerWithOrdinal>, 2> markersSortedByKmerId;
    getMarkersSortedByKmerId(orientedReadId0, markersSortedByKmerId[0]);

    // Loop over all alignments involving this oriented read.
    size_t goodAlignmentCount = 0;
    for(const uint64_t i: alignmentTable[orientedReadId0.getValue()]) {
        const AlignmentData& ad = alignmentData[i];

        // Get the other oriented read involved in this alignment.
        const OrientedReadId orientedReadId1 = ad.getOther(orientedReadId0);

        // Get the markers for orientedReadId1.
        getMarkersSortedByKmerId(orientedReadId1, markersSortedByKmerId[1]);

        // Compute the alignment.
        AlignmentGraph graph;
        Alignment alignment;
        AlignmentInfo alignmentInfo;
        const bool debug = false;
        alignOrientedReads(
            markersSortedByKmerId,
            maxSkip, maxDrift, maxMarkerFrequency, debug, graph, alignment, alignmentInfo);

        uint32_t leftTrim;
        uint32_t rightTrim;
        tie(leftTrim, rightTrim) = alignmentInfo.computeTrim();

        cout << orientedReadId0 << " " << orientedReadId1 << " " << alignmentInfo.markerCount;
        if(alignmentInfo.markerCount) {
            cout << " " << leftTrim << " " << rightTrim;
            if(alignmentInfo.markerCount >= minAlignedMarkerCount && leftTrim<=maxTrim && rightTrim<=maxTrim) {
                cout << " good";
                ++goodAlignmentCount;
            }
        }
        cout << endl;

    }
    cout << "Found " << goodAlignmentCount << " alignments out of ";
    cout << alignmentTable[orientedReadId0.getValue()].size() << "." << endl;

}



// Compute an alignment for each alignment candidate.
// Store summary information for the ones that are good enough,
// without storing details of the alignment.
void Assembler::computeAlignments(

    // Marker frequency threshold.
    // When computing an alignment between two oriented reads,
    // marker kmers that appear more than this number of times
    // in either of the two oriented reads are discarded
    // (in both oriented reads).
    uint32_t maxMarkerFrequency,

    // The maximum ordinal skip to be tolerated between successive markers
    // in the alignment.
    size_t maxSkip,

    // The maximum relative ordinal drift to be tolerated between successive markers
    // in the alignment.
    size_t maxDrift,

    // Minimum number of alignment markers for an alignment to be used.
    size_t minAlignedMarkerCount,

    // Maximum left/right trim (in bases) for an alignment to be used.
    size_t maxTrim,

    // Number of threads. If zero, a number of threads equal to
    // the number of virtual processors is used.
    size_t threadCount
)
{
    const auto tBegin = steady_clock::now();
    cout << timestamp << "Begin computing alignments for ";
    cout << alignmentCandidates.candidates.size() << " alignment candidates." << endl;

    // Check that we have what we need.
    checkReadsAreOpen();
    checkKmersAreOpen();
    checkMarkersAreOpen();
    checkAlignmentCandidatesAreOpen();

    // Store parameters so they are accessible to the threads.
    auto& data = computeAlignmentsData;
    data.maxMarkerFrequency = maxMarkerFrequency;
    data.maxSkip = maxSkip;
    data.maxDrift = maxDrift;
    data.minAlignedMarkerCount = minAlignedMarkerCount;
    data.maxTrim = maxTrim;

    // Adjust the numbers of threads, if necessary.
    if(threadCount == 0) {
        threadCount = std::thread::hardware_concurrency();
    }

    // Pick the batch size for computing alignments.
    size_t batchSize = 10000;
    if(batchSize > alignmentCandidates.candidates.size()/threadCount) {
        batchSize = alignmentCandidates.candidates.size()/threadCount;
    }
    if(batchSize == 0) {
        batchSize = 1;
    }


    // Compute the alignments.
    data.threadAlignmentData.resize(threadCount);
    cout << timestamp << "Alignment computation begins." << endl;
    setupLoadBalancing(alignmentCandidates.candidates.size(), batchSize);
    runThreads(&Assembler::computeAlignmentsThreadFunction, threadCount);
    cout << timestamp << "Alignment computation completed." << endl;



    // Store alignmentInfos found by each thread in the global alignmentInfos.
    cout << timestamp << "Storing the alignment info objects." << endl;
    alignmentData.createNew(largeDataName("AlignmentData"), largeDataPageSize);
    for(size_t threadId=0; threadId<threadCount; threadId++) {
        const vector<AlignmentData>& threadAlignmentData = data.threadAlignmentData[threadId];
        for(const AlignmentData& ad: threadAlignmentData) {
            alignmentData.push_back(ad);
        }
    }
    cout << timestamp << "Creating alignment table." << endl;
    computeAlignmentTable();

    const auto tEnd = steady_clock::now();
    const double tTotal = seconds(tEnd - tBegin);
    cout << timestamp << "Computation of alignments ";
    cout << "completed in " << tTotal << " s." << endl;
}



void Assembler::computeAlignmentsThreadFunction(size_t threadId)
{

    array<OrientedReadId, 2> orientedReadIds;
    array<OrientedReadId, 2> orientedReadIdsOppositeStrand;
    array<vector<MarkerWithOrdinal>, 2> markersSortedByKmerId;
    AlignmentGraph graph;
    Alignment alignment;
    AlignmentInfo alignmentInfo;

    const bool debug = false;
    auto& data = computeAlignmentsData;
    const uint32_t maxMarkerFrequency = data.maxMarkerFrequency;
    const size_t maxSkip = data.maxSkip;
    const size_t maxDrift = data.maxDrift;
    const size_t minAlignedMarkerCount = data.minAlignedMarkerCount;
    const size_t maxTrim = data.maxTrim;

    vector<AlignmentData>& threadAlignmentData = data.threadAlignmentData[threadId];

    uint64_t begin, end;
    while(getNextBatch(begin, end)) {
        if((begin % 1000000) == 0){
            std::lock_guard<std::mutex> lock(mutex);
            cout << timestamp << "Working on alignment " << begin;
            cout << " of " << alignmentCandidates.candidates.size() << endl;
        }

        for(size_t i=begin; i!=end; i++) {
            const OrientedReadPair& candidate = alignmentCandidates.candidates[i];
            SHASTA_ASSERT(candidate.readIds[0] < candidate.readIds[1]);

            // Get the oriented read ids, with the first one on strand 0.
            orientedReadIds[0] = OrientedReadId(candidate.readIds[0], 0);
            orientedReadIds[1] = OrientedReadId(candidate.readIds[1], candidate.isSameStrand ? 0 : 1);

            // Get the oriented read ids for the opposite strand.
            orientedReadIdsOppositeStrand = orientedReadIds;
            orientedReadIdsOppositeStrand[0].flipStrand();
            orientedReadIdsOppositeStrand[1].flipStrand();


            // out << timestamp << "Working on " << i << " " << orientedReadIds[0] << " " << orientedReadIds[1] << endl;

            // Get the markers for the two oriented reads in this candidate.
            for(size_t j=0; j<2; j++) {
                getMarkersSortedByKmerId(orientedReadIds[j], markersSortedByKmerId[j]);
            }

            // Compute the Alignment.
            alignOrientedReads(
                markersSortedByKmerId,
                maxSkip, maxDrift, maxMarkerFrequency, debug, graph, alignment, alignmentInfo);

            // If the alignment has too few markers skip it.
            if(alignment.ordinals.size() < minAlignedMarkerCount) {
                continue;
            }

            // If the alignment has too much trim, skip it.
            uint32_t leftTrim;
            uint32_t rightTrim;
            tie(leftTrim, rightTrim) = alignmentInfo.computeTrim();
            if(leftTrim>maxTrim || rightTrim>maxTrim) {
                continue;
            }

            // If getting here, this is a good alignment.
            threadAlignmentData.push_back(AlignmentData(candidate, alignmentInfo));
        }
    }
}



// Compute alignmentTable from alignmentData.
// This could be made multithreaded if it becomes a bottleneck.
void Assembler::computeAlignmentTable()
{
    alignmentTable.createNew(largeDataName("AlignmentTable"), largeDataPageSize);
    alignmentTable.beginPass1(ReadId(2 * reads.size()));
    for(const AlignmentData& ad: alignmentData) {
        const auto& readIds = ad.readIds;
        OrientedReadId orientedReadId0(readIds[0], 0);
        OrientedReadId orientedReadId1(readIds[1], ad.isSameStrand ? 0 : 1);
        alignmentTable.incrementCount(orientedReadId0.getValue());
        alignmentTable.incrementCount(orientedReadId1.getValue());
        orientedReadId0.flipStrand();
        orientedReadId1.flipStrand();
        alignmentTable.incrementCount(orientedReadId0.getValue());
        alignmentTable.incrementCount(orientedReadId1.getValue());
    }
    alignmentTable.beginPass2();
    for(uint32_t i=0; i<alignmentData.size(); i++) {
        const AlignmentData& ad = alignmentData[i];
        const auto& readIds = ad.readIds;
        OrientedReadId orientedReadId0(readIds[0], 0);
        OrientedReadId orientedReadId1(readIds[1], ad.isSameStrand ? 0 : 1);
        alignmentTable.store(orientedReadId0.getValue(), i);
        alignmentTable.store(orientedReadId1.getValue(), i);
        orientedReadId0.flipStrand();
        orientedReadId1.flipStrand();
        alignmentTable.store(orientedReadId0.getValue(), i);
        alignmentTable.store(orientedReadId1.getValue(), i);
    }
    alignmentTable.endPass2();



    // Sort each section of the alignment table by OrientedReadId.
    vector< pair<OrientedReadId, uint32_t> > v;
    for(ReadId readId0=0; readId0<reads.size(); readId0++) {
        for(Strand strand0=0; strand0<2; strand0++) {
            const OrientedReadId orientedReadId0(readId0, strand0);

            // Access the section of the alignment table for this oriented read.
            const MemoryAsContainer<uint32_t> alignmentTableSection =
                alignmentTable[orientedReadId0.getValue()];

            // Store pairs(OrientedReadId, alignmentIndex).
            v.clear();
            for(uint32_t alignmentIndex: alignmentTableSection) {
                const AlignmentData& alignment = alignmentData[alignmentIndex];
                const OrientedReadId orientedReadId1 = alignment.getOther(orientedReadId0);
                v.push_back(make_pair(orientedReadId1, alignmentIndex));
            }

            // Sort.
            sort(v.begin(), v.end());

            // Store the sorted alignmentIndex.
            for(size_t i=0; i<v.size(); i++) {
                alignmentTableSection[i] = v[i].second;
            }
        }
    }


}



void Assembler::accessAlignmentData()
{
    alignmentData.accessExistingReadOnly(largeDataName("AlignmentData"));
    alignmentTable.accessExistingReadOnly(largeDataName("AlignmentTable"));
}



void Assembler::checkAlignmentDataAreOpen()
{
    if(!alignmentData.isOpen || !alignmentTable.isOpen()) {
        throw runtime_error("Alignment data are not accessible.");
    }
}



// Find in the alignment table the alignments involving
// a given oriented read, and return them with the correct
// orientation (this may involve a swap and/or reverse complement
// of the AlignmentInfo stored in the alignmentTable).
vector< pair<OrientedReadId, shasta::AlignmentInfo> >
    Assembler::findOrientedAlignments(OrientedReadId orientedReadId0Argument) const
{
    const ReadId readId0 = orientedReadId0Argument.getReadId();
    const ReadId strand0 = orientedReadId0Argument.getStrand();

    vector< pair<OrientedReadId, AlignmentInfo> > result;

    // Loop over alignment involving this read, as stored in the
    // alignment table.
    const auto alignmentTable0 = alignmentTable[orientedReadId0Argument.getValue()];
    for(const auto i: alignmentTable0) {
        const AlignmentData& ad = alignmentData[i];

        // Get the oriented read ids that the AlignmentData refers to.
        OrientedReadId orientedReadId0(ad.readIds[0], 0);
        OrientedReadId orientedReadId1(ad.readIds[1], ad.isSameStrand ? 0 : 1);
        AlignmentInfo alignmentInfo = ad.info;

        // Swap oriented reads, if necessary.
        if(orientedReadId0.getReadId() != readId0) {
            swap(orientedReadId0, orientedReadId1);
            alignmentInfo.swap();
        }
        SHASTA_ASSERT(orientedReadId0.getReadId() == readId0);

        // Reverse complement, if necessary.
        if(orientedReadId0.getStrand() != strand0) {
            orientedReadId0.flipStrand();
            orientedReadId1.flipStrand();
            alignmentInfo.reverseComplement();
        }
        SHASTA_ASSERT(orientedReadId0.getStrand() == strand0);
        SHASTA_ASSERT(orientedReadId0 == orientedReadId0Argument);

        result.push_back(make_pair(orientedReadId1, alignmentInfo));
    }
    return result;
}



// Flag palindromic reads.
void Assembler::flagPalindromicReads(
    uint32_t maxSkip,
    uint32_t maxDrift,
    uint32_t maxMarkerFrequency,
    double alignedFractionThreshold,
    double nearDiagonalFractionThreshold,
    uint32_t deltaThreshold,
    size_t threadCount)
{
    cout << timestamp << "Finding palindromic reads." << endl;

    // Adjust the numbers of threads, if necessary.
    if(threadCount == 0) {
        threadCount = std::thread::hardware_concurrency();
    }

    // Store the parameters so all threads can see them.
    flagPalindromicReadsData.maxSkip = maxSkip;
    flagPalindromicReadsData.maxDrift = maxDrift;
    flagPalindromicReadsData.maxMarkerFrequency = maxMarkerFrequency;
    flagPalindromicReadsData.alignedFractionThreshold = alignedFractionThreshold;
    flagPalindromicReadsData.nearDiagonalFractionThreshold = nearDiagonalFractionThreshold;
    flagPalindromicReadsData.deltaThreshold = deltaThreshold;

    // Reset all palindromic flags.
    const ReadId readCount = ReadId(readFlags.size());
    for(ReadId readId=0; readId<readCount; readId++) {
        readFlags[readId].isPalindromic = 0;
    }

    // Do it in parallel.
    setupLoadBalancing(readCount, 1000);
    runThreads(&Assembler::flagPalindromicReadsThreadFunction, threadCount);

    // Count the reads flagged as palindromic.
    size_t palindromicReadCount = 0;
    for(ReadId readId=0; readId<readCount; readId++) {
        if(readFlags[readId].isPalindromic) {
            ++palindromicReadCount;
        }
    }
    assemblerInfo->palindromicReadCount = palindromicReadCount;
    cout << timestamp << "Flagged " << palindromicReadCount <<
        " reads as palindromic out of " << readCount << " total." << endl;
    cout << "Palindromic fraction is " <<
        double(palindromicReadCount)/double(readCount) << endl;


    // Write a csv file with the list of palindromic reads.
    // This should not too big as the typical rate of
    // palindromic reads is around 1e-4.
    ofstream csvOut("PalindromicReads.csv");
    for(ReadId readId=0; readId<readCount; readId++) {
        if(readFlags[readId].isPalindromic) {
            csvOut << readId << "\n";
        }
    }
}



void Assembler::flagPalindromicReadsThreadFunction(size_t threadId)
{

    // Work areas used inside the loop and defined here
    // to reduce memory allocation activity.
    AlignmentGraph graph;
    Alignment alignment;
    AlignmentInfo alignmentInfo;
    array<vector<MarkerWithOrdinal>, 2> markersSortedByKmerId;

    // Make local copies of the parameters.
    const uint32_t maxSkip = flagPalindromicReadsData.maxSkip;
    const uint32_t maxDrift = flagPalindromicReadsData.maxDrift;
    const uint32_t maxMarkerFrequency = flagPalindromicReadsData.maxMarkerFrequency;
    const double alignedFractionThreshold = flagPalindromicReadsData.alignedFractionThreshold;
    const double nearDiagonalFractionThreshold = flagPalindromicReadsData.nearDiagonalFractionThreshold;
    const uint32_t deltaThreshold = flagPalindromicReadsData.deltaThreshold;


    // Loop over all batches assigned to this thread.
    uint64_t begin, end;
    while(getNextBatch(begin, end)) {
        if((begin%1000000) == 0) {
            std::lock_guard<std::mutex> lock(mutex);
            cout << timestamp << begin << "/" << readFlags.size() << endl;
        }

        // Loop over all reads in this batch.
        for(ReadId readId=ReadId(begin); readId!=ReadId(end); readId++) {

            // Get markers sorted by KmerId for this read and its reverse complement.
            for(Strand strand=0; strand<2; strand++) {
                getMarkersSortedByKmerId(OrientedReadId(readId, strand), markersSortedByKmerId[strand]);
            }

            // Compute a marker alignment of this read versus its reverse complement.
            alignOrientedReads(markersSortedByKmerId, maxSkip, maxDrift, maxMarkerFrequency, false,
                graph, alignment, alignmentInfo);

            // If the alignment has too few markers, skip it.
            const size_t alignedMarkerCount = alignment.ordinals.size();
            const size_t totalMarkerCount = markersSortedByKmerId[0].size();
            const double alignedFraction = double(alignedMarkerCount)/double(totalMarkerCount);
            if(alignedFraction < alignedFractionThreshold) {
                continue;
            }

            // If the alignment has too few markers near the diagonal, skip it.
            size_t nearDiagonalMarkerCount = 0;
            for(size_t i=0; i<alignment.ordinals.size(); i++) {
                const array<uint32_t, 2>& ordinals = alignment.ordinals[i];
                const int32_t ordinal0 = int32_t(ordinals[0]);
                const int32_t ordinal1 = int32_t(ordinals[1]);
                const uint32_t delta = abs(ordinal0 - ordinal1);
                if(delta < deltaThreshold) {
                    nearDiagonalMarkerCount++;
                }
            }
            const double nearDiagonalFraction = double(nearDiagonalMarkerCount)/double(totalMarkerCount);
            if(nearDiagonalFraction < nearDiagonalFractionThreshold) {
                continue;
            }

            // If we got here, mark the read as palindromic.
            readFlags[readId].isPalindromic = 1;

        }
    }
}



void Assembler::analyzeAlignmentMatrix(
    ReadId readId0, Strand strand0,
    ReadId readId1, Strand strand1)
{
    // Get the oriented reads.
    const OrientedReadId orientedReadId0(readId0, strand0);
    const OrientedReadId orientedReadId1(readId1, strand1);

    // Get the markers sorted by kmerId.
    vector<MarkerWithOrdinal> markers0;
    vector<MarkerWithOrdinal> markers1;
    getMarkersSortedByKmerId(orientedReadId0, markers0);
    getMarkersSortedByKmerId(orientedReadId1, markers1);

    // Some iterators we will need.
    using MarkerIterator = vector<MarkerWithOrdinal>::const_iterator;
    const MarkerIterator begin0 = markers0.begin();
    const MarkerIterator end0   = markers0.end();
    const MarkerIterator begin1 = markers1.begin();
    const MarkerIterator end1   = markers1.end();

    // The number of markers in each oriented read.
    const int64_t n0 = end0 - begin0;
    const int64_t n1 = end1 - begin1;

    // We will use coordinates
    // x = ordinal0 + ordinal1
    // y = ordinal0 - ordinal1 (offset)
    // In these coordinates, diagonals in the alignment matrix
    // are lines of constant y and so they become horizontal.
    const int64_t xMin = 0;
    const int64_t xMax = n0 + n1 - 2;
    const int64_t yMin = -n1;
    const int64_t yMax = n0 - 1;
    const int64_t nx= xMax -xMin + 1;
    const int64_t ny= yMax -yMin + 1;

    // Create a histogram in cells of size (dx, dy).
    const int64_t dx = 100;
    const int64_t dy = 20;
    const int64_t nxCells = (nx-1)/dx + 1;
    const int64_t nyCells = (ny-1)/dy + 1;
    vector< vector<uint64_t> > histogram(nxCells, vector<uint64_t>(nyCells, 0));
    cout << "nxCells " << nxCells << endl;
    cout << "nyCells " << nyCells << endl;



    // Joint loop over the markers, looking for common k-mer ids.
    auto it0 = begin0;
    auto it1 = begin1;
    while(it0!=end0 && it1!=end1) {
        if(it0->kmerId < it1->kmerId) {
            ++it0;
        } else if(it1->kmerId < it0->kmerId) {
            ++it1;
        } else {

            // We found a common k-mer id.
            const KmerId kmerId = it0->kmerId;


            // This k-mer could appear more than once in each of the oriented reads,
            // so we need to find the streak of this k-mer in kmers0 and kmers1.
            MarkerIterator it0Begin = it0;
            MarkerIterator it1Begin = it1;
            MarkerIterator it0End = it0Begin;
            MarkerIterator it1End = it1Begin;
            while(it0End!=end0 && it0End->kmerId==kmerId) {
                ++it0End;
            }
            while(it1End!=end1 && it1End->kmerId==kmerId) {
                ++it1End;
            }


            // Loop over pairs in the streaks.
            for(MarkerIterator jt0=it0Begin; jt0!=it0End; ++jt0) {
                const int64_t ordinal0 = jt0->ordinal;
                for(MarkerIterator jt1=it1Begin; jt1!=it1End; ++jt1) {
                    const int64_t ordinal1 = int64_t(jt1->ordinal);

                    const int64_t x = ordinal0 + ordinal1;
                    const int64_t y = ordinal0 - ordinal1;
                    const int64_t ix = (x-xMin) / dx;
                    const int64_t iy = (y-yMin) / dy;
                    SHASTA_ASSERT(ix >= 0);
                    SHASTA_ASSERT(iy >= 0);
                    SHASTA_ASSERT(ix < nxCells);
                    SHASTA_ASSERT(iy < nyCells);

                    ++histogram[ix][iy];
                }
            }


            // Continue joint loop over k-mers.
            it0 = it0End;
            it1 = it1End;
        }
    }

    ofstream csv("Histogram.csv");
    PngImage image = PngImage(int(nxCells), int(nyCells));
    uint64_t activeCellCount = 0;
    for(int64_t iy=0; iy<nyCells; iy++) {
        for(int64_t ix=0; ix<nxCells; ix++) {
            const int64_t frequency = histogram[ix][iy];
            if(frequency > 0) {
                csv << frequency;
            }
            if(frequency >= 10) {
                ++activeCellCount;
            }
            // const int r = (frequency <= 10) ? 0 : min(int(255), int(10*frequency));
            const int r = (frequency>=10) ? 255 : 0;
            // const int r = min(int(255), int(10*frequency));
            const int g = r;
            const int b = r;
            image.setPixel(int(ix), int(iy), r, g, b);
            csv << ",";
        }
        csv << "\n";
    }
    image.write("Histogram.png");
    cout << activeCellCount << " active cells out of " << nxCells*nyCells << endl;


}
