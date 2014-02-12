// fastgene.cpp	- fast genetic algorithm code
// \copyright 1992 DCBlaha.  Distributed under the GPL.

#include "FastGene.h"

#define MYRAND 0	// Use my random number generator or 'C' rand()

#if(MYRAND)
#define RAND_MAX 0x7fff
#else
#include <stdlib.h>	// Declares rand()
#endif
#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include <ctype.h>


/*
* This random number generation routine can be used in the case that the 'C'
* library rand routine is not good enough.
*
* This code came from Computer Language, 10-89, page 59, T.A.Elkins.
* Original source was assembly;
*/
#if(MYRAND)
static int customRand(void)
    {
    // Initial seeds - these can be changed
    static unsigned int h1 = 7397;
    static unsigned int h2 = 29447;
    static unsigned int h3 = 802;
    // Do not change these
    const unsigned int f1 = 179;
    const unsigned int f2 = 183;
    const unsigned int f3 = 182;
    const unsigned int m1 = 32771;
    const unsigned int m2 = 32779;
    const unsigned int m3 = 32783;
    register unsigned int res;
    register unsigned char t;
    register unsigned int temp1, temp2, temp3;

    do
	{
	t = 0;
	temp1 = (h1 * f1) % m1;
	h1 = temp1;
	temp1--;
	if(temp1 >= 32767)
	    t++;
	temp2 = (h2 * f2) % m2;
	h2 = temp2;
	temp2--;
	if(temp2 >= 32767)
	t++;
	res = temp1 + temp2;
	temp3 = (h3 * f3) % m3;
	h3 = temp3;
	temp3--;
	if(temp3 >= 32767)
	t++;
	} while(t != 0);
    res += temp3;
    res = res & 32767;
    return(res);
    }
#endif

/// Generate a random number between 0 and numpossibles-1
static int randmax(int numpossibles)
    {
    return((int)(((long)rand() * numpossibles) / ((long)RAND_MAX + 1)));
    }

GeneValue GenePool::randRange(int min, int max)
    {
    return(static_cast<GeneValue>(
	    (min + ((long)rand() * (max - min + 1)) / ((long)RAND_MAX + 1))
	    ));
    }

void GenePool::initialize(int genebytes, int numberofgenes, double crossoverrate,
    double mutaterate, int minrand, int maxrand)
    {
    // Copy the information we want to keep about all of the genes
    numgenes = numberofgenes;
    genesize = genebytes;
    muterate = mutaterate;
    min = minrand;
    max = maxrand;

    // 	static_assert(crossoverrate < 0.5);	// Crossover rate must be less than .5, using .35.
    if(crossoverrate >= 0.5)
    	{
    	crossoverrate = 0.35;
    	}
    int numbestgenes = (int)(numgenes * crossoverrate);
    if(numbestgenes & 1)	// Make numbestgenes an even number
	numbestgenes++;

    // Allocate space for the genes
    genes.resize((genesize + sizeof(QualityType)) * numgenes);
    // Allocate space to help in computing the crossover genes
    bestgenes.resize(numbestgenes);
    worstgenes.resize(numbestgenes);

    // Initialize the genes with random data
    for(int i=0; i<numgenes; i++)
	randomizeGene(i);
    }


void GenePool::randomizeGene(int index)
	{
	for(int i=0; i<genesize; i+=sizeof(GeneValue))
	    {
	    randomizeGeneValue(index, i);
	    }
	}

void GenePool::randomizeGeneValue(int index, int offset)
    {
    offset = geneValueBoundary(offset);
    GeneValue val = randRange(min, max);
    setValue(index, offset, val);
    }

void GenePool::singleGeneration()
    {
    // Find quality values for all genes
    computeQuality();
    // Build lists of best and worst genes to aid on computing crossover
    buildBestWorstList();
    // Combine the genes that are the best and overwrite the worst ones
    crossover();
    // Mutate a certain number of genes
    mutate();
    }

void GenePool::computeQuality()
    {
    setupQualityEachGeneration();
    for(int i=0; i<numgenes; i++)
	{
	setGeneQuality(i, calculateSingleGeneQuality(i));
	}
    }

void GenePool::getQualityHistogram(QualityHistogram &qualities) const
    {
    QualityType maxquality = 0;
    for(int i=0; i<numgenes; i++)
	{
	QualityType q = getGeneQuality(i);
	if(q > maxquality)
	    maxquality = q;
	}
    qualities.resize(maxquality+1);
    // Make a histogram of the qualities of the genes
    for(int i=0; i<numgenes; i++)
	{
	qualities[getGeneQuality(i)]++;
	}
    }

void GenePool::buildBestWorstList()
    {
    QualityType bestquality, worstquality;
    int numQualityBins;

    QualityHistogram qualities;
    getQualityHistogram(qualities);
    // Find the quality value to use to determine the worst genes
    numQualityBins = 0;
    worstquality = qualities.size() - 1;
    for (size_t i = 0; i < qualities.size() && worstquality == qualities.size() - 1; i++)
	{
	numQualityBins += qualities[i];
	if (static_cast<unsigned>(numQualityBins) > bestgenes.size())
	    worstquality = i;
	}

    // Find the quality value to use to determine the best genes
    numQualityBins = 0;
    bestquality = 0;
    for (int i = qualities.size() - 1; i >= 0 && bestquality == 0; i--)
	{
	numQualityBins += qualities[i];
	if (static_cast<unsigned>(numQualityBins) > bestgenes.size())
	    bestquality = i;
	}
//printf("worstcut %d bestcut %d\n", worstquality, bestquality);

    // Build a list of best genes
    int besti = 0;
    for (int i = 0; i < numgenes; i++)
	{
	if(getGeneQuality(i) >= bestquality)
	    {
	    if(static_cast<unsigned>(besti) < bestgenes.size())
		bestgenes[besti++] = getGene(i);
	    else
		break;
	    }
	}

    // Build a list of worst genes
    int worsti = 0;
    for (int i = 0; i < numgenes; i++)
	{
	if(getGeneQuality(i) <= worstquality)
	    {
	    if(static_cast<unsigned>(worsti) < worstgenes.size())
		worstgenes[worsti++] = getGene(i);
	    else
		break;
	    }
	}
    }

void GenePool::crossover()
    {
    int genesRemaining = bestgenes.size();
    int dstgene = 0;
    // Each time, take 2 good source genes, cross them, and put them
    // into the 2 worst genes.
    while (genesRemaining)
	{
	int splitpos = geneValueBoundary(randmax(genesize));
	int srcgene1 = randmax(genesRemaining);
	int srcgene2 = randmax(genesRemaining - 1);
	if (srcgene1 == srcgene2)
	    {
	    if (srcgene1 < genesRemaining - 1)
		++srcgene1;
	    else
		--srcgene1;
	    }

	GenePtr srcgene1str = bestgenes[srcgene1];
	GenePtr srcgene2str = bestgenes[srcgene2];

	// Copy parts of 2 genes to 2 new genes.
	GenePtr dstgenestr = worstgenes[dstgene++];
	memcpy(dstgenestr, srcgene1str, splitpos);
	memcpy(dstgenestr + splitpos, srcgene2str + splitpos,
		genesize - splitpos);

	dstgenestr = worstgenes[dstgene++];
	memcpy(dstgenestr, srcgene2str, splitpos);
	memcpy(dstgenestr + splitpos, srcgene1str + splitpos,
		genesize - splitpos);

	/* Copy the genes at the end of the list into the genes that were
	 * already crossed over since the list of remaining best genes is
	 * getting shorter.
	 */
	bestgenes[srcgene1] = bestgenes[genesRemaining - 1];
	bestgenes[srcgene2] = bestgenes[genesRemaining - 2];

	genesRemaining -= 2;
	}
    }

void GenePool::mutate()
    {
    int totalbytes = genesize * numgenes;
    int mutebits = (int) ((muterate * 8) * totalbytes);
    for(int i = 0; i < mutebits; i++)
	{
	// Get the offset of a byte in any portion of any gene
	int gene = randmax(numgenes);
	int offset = randmax(genesize);
	randomizeGeneValue(gene, offset);

	/*
	 // Get the address of a byte in any portion of any gene
	 byteaddr = genes + (sizeof(GeneHeader) + genesize) *
	 randmax(numgenes) + randmax(genesize) + sizeof(GeneHeader);

	 // Invert one bit in the byte
	 newval = (unsigned char)(*byteaddr ^ (1 << randmax(8)));
	 if(newval >= min && newval <= max)
	 *byteaddr = newval;
	 */
	}
    }

int GenePool::getBestGeneIndex()
    {
    computeQuality();

    QualityHistogram qualities;
    getQualityHistogram(qualities);
    QualityType bestQ = 0;

    for(int i=qualities.size()-1; i>=0 && !bestQ; i--)
	{
	if(qualities[i])
	    bestQ = i;
	}
    // Most if the time, the first gene is the best.
    int bestGeneI = 0;
    for(int i=0; i<numgenes; i++)
	{
	if(getGeneQuality(i) == bestQ)
	    {
	    bestGeneI = i;
	    break;
	    }
	}
    return bestGeneI;
    }


#if(0)
#define GENEBYTES 10
// This will optimize for the gene with the highest average value of bytes.
// The randomizer, and crossover is working on GeneValue sized parts of each gene.
QualityType calculateQuality(int geneIndex)
    {
    int value = 0;
    for(int i=0; i<GENEBYTES; i++)
	value += getGene(geneIndex)[i];
    return(value / GENEBYTES);
    }
main(unsigned int argc, char *argv[])
    {
    double crossrate = 0.35;
    double muterate = 0.005;
    int iterations = 10;
    int numgenes = 100;
    GenePool genes;
    genes.initialize(GENEBYTES, numgenes, crossrate, muterate);

    for(i=0; i<iterations; i++)
	{
	genes.singleGeneration();
	QualityHistogram qualities;
	genes.getQualityHistogram(qualities);
	int worst = 0, best = 0;

	for(int i=qualities.size()-1; i>=0 && !best; i--)
	    {
	    if(qualities[i])
		best = i;
	    }
	for(int i=0; i<qualities.size()-1 && !worst; i++)
	    {
	    if(qualities[i])
		worst = i;
	    }
	printf("worst %d best %d\n", worst, best);
	}
    }
#endif