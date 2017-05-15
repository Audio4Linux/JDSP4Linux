#ifndef G72X_HEADER_FILE
#define	G72X_HEADER_FILE

/*
** Number of samples per block to process.
** Must be a common multiple of possible bits per sample : 2, 3, 4, 5 and 8.
*/
#define	G72x_BLOCK_SIZE		(3 * 5 * 8)

/*
**	Identifiers for the differing kinds of G72x ADPCM codecs.
**	The identifiers also define the number of encoded bits per sample.
*/

enum
{	G723_16_BITS_PER_SAMPLE = 2,
	G723_24_BITS_PER_SAMPLE = 3,
	G723_40_BITS_PER_SAMPLE = 5,

	G721_32_BITS_PER_SAMPLE = 4,
	G721_40_BITS_PER_SAMPLE = 5,

	G723_16_SAMPLES_PER_BLOCK = G72x_BLOCK_SIZE,
	G723_24_SAMPLES_PER_BLOCK = G723_24_BITS_PER_SAMPLE * (G72x_BLOCK_SIZE / G723_24_BITS_PER_SAMPLE),
	G723_40_SAMPLES_PER_BLOCK = G723_40_BITS_PER_SAMPLE * (G72x_BLOCK_SIZE / G723_40_BITS_PER_SAMPLE),

	G721_32_SAMPLES_PER_BLOCK = G72x_BLOCK_SIZE,
	G721_40_SAMPLES_PER_BLOCK = G721_40_BITS_PER_SAMPLE * (G72x_BLOCK_SIZE / G721_40_BITS_PER_SAMPLE),

	G723_16_BYTES_PER_BLOCK = (G723_16_BITS_PER_SAMPLE * G72x_BLOCK_SIZE) / 8,
	G723_24_BYTES_PER_BLOCK = (G723_24_BITS_PER_SAMPLE * G72x_BLOCK_SIZE) / 8,
	G723_40_BYTES_PER_BLOCK = (G723_40_BITS_PER_SAMPLE * G72x_BLOCK_SIZE) / 8,

	G721_32_BYTES_PER_BLOCK = (G721_32_BITS_PER_SAMPLE * G72x_BLOCK_SIZE) / 8,
	G721_40_BYTES_PER_BLOCK = (G721_40_BITS_PER_SAMPLE * G72x_BLOCK_SIZE) / 8
} ;

/* Forward declaration of of g72x_state. */

struct g72x_state ;

/* External function definitions. */

struct g72x_state * g72x_reader_init (int codec, int *blocksize, int *samplesperblock) ;
struct g72x_state * g72x_writer_init (int codec, int *blocksize, int *samplesperblock) ;
/*
**	Initialize the ADPCM state table for the given codec.
**	Return 0 on success, 1 on fail.
*/

int g72x_decode_block (struct g72x_state *pstate, const unsigned char *block, short *samples) ;
/*
**	The caller fills data->block with data->bytes bytes before calling the
**	function. The value data->bytes must be an integer multiple of
**	data->blocksize and be <= data->max_bytes.
**	When it returns, the caller can read out data->samples samples.
*/

int g72x_encode_block (struct g72x_state *pstate, short *samples, unsigned char *block) ;
/*
**	The caller fills state->samples some integer multiple data->samples_per_block
**	(up to G72x_BLOCK_SIZE) samples before calling the function.
**	When it returns, the caller can read out bytes encoded bytes.
*/

#endif /* !G72X_HEADER_FILE */

