#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

int uleb128_encode_small(uint8_t *out, uint32_t n)
{
    assert(n <= 0x3fff);
    if (n < 0x80) {
        *out = n;
        return 1;
    } else {
        *out++ = (n & 0x7f) | 0x80;
        *out = n >> 7;
        return 2;
    }
}

int uleb128_decode_small(const uint8_t *in, uint32_t *n)
{
    if (!(*in & 0x80)) {
        *n = *in;
        return 1;
    } else {
        *n = *in++ & 0x7f;
        /* we exceed 14 bit number */
        if (*in & 0x80) {
            return -1;
        }
        *n |= *in << 7;
        return 2;
    }
}

int xbzrle_encode_buffer(uint8_t *old_buf, uint8_t *new_buf, int slen,
                         uint8_t *dst, int dlen)
{
    uint32_t zrun_len = 0, nzrun_len = 0;
    int d = 0, i = 0;
    long res;
    uint8_t *nzrun_start = NULL;

    assert(!(((uintptr_t)old_buf | (uintptr_t)new_buf | slen) %
               sizeof(long)));

    while (i < slen) {
        /* overflow */
        if (d + 2 > dlen) {
            return -1;
        }

        /* not aligned to sizeof(long) */
        res = (slen - i) % sizeof(long);
        while (res && old_buf[i] == new_buf[i]) {
            zrun_len++;
            i++;
            res--;
        }

        /* word at a time for speed */
        if (!res) {
            while (i < slen &&
                   (*(long *)(old_buf + i)) == (*(long *)(new_buf + i))) {
                i += sizeof(long);
                zrun_len += sizeof(long);
            }

            /* go over the rest */
            while (i < slen && old_buf[i] == new_buf[i]) {
                zrun_len++;
                i++;
            }
        }

        /* buffer unchanged */
        if (zrun_len == slen) {
            return 0;
        }

        /* skip last zero run */
        if (i == slen) {
            return d;
        }

        d += uleb128_encode_small(dst + d, zrun_len);

        zrun_len = 0;
        nzrun_start = new_buf + i;

        /* overflow */
        if (d + 2 > dlen) {
            return -1;
        }
        /* not aligned to sizeof(long) */
        res = (slen - i) % sizeof(long);
        while (res && old_buf[i] != new_buf[i]) {
            i++;
            nzrun_len++;
            res--;
        }

        /* word at a time for speed, use of 32-bit long okay */
        if (!res) {
            /* truncation to 32-bit long okay */
            unsigned long mask = (unsigned long)0x0101010101010101ULL;
            while (i < slen) {
                unsigned long xor;
                xor = *(unsigned long *)(old_buf + i)
                    ^ *(unsigned long *)(new_buf + i);
                if ((xor - mask) & ~xor & (mask << 7)) {
                    /* found the end of an nzrun within the current long */
                    while (old_buf[i] != new_buf[i]) {
                        nzrun_len++;
                        i++;
                    }
                    break;
                } else {
                    i += sizeof(long);
                    nzrun_len += sizeof(long);
                }
            }
        }

        d += uleb128_encode_small(dst + d, nzrun_len);
        /* overflow */
        if (d + nzrun_len > dlen) {
            return -1;
        }
        memcpy(dst + d, nzrun_start, nzrun_len);
        d += nzrun_len;
        nzrun_len = 0;
    }

    return d;
}

void leb128_encode_decode()
{
	uint8_t con[2] = {0};
	uint32_t val;

	uleb128_encode_small(con, 0);
	printf("     0 encoded to:%x %x\n", con[1], con[0]);
	uleb128_decode_small(con, &val);
	printf("result decoded to: %d\n", val);

	uleb128_encode_small(con, 10);
	printf("    10 encoded to:%x %x\n", con[1], con[0]);
	uleb128_decode_small(con, &val);
	printf("result decoded to: %d\n", val);

	uleb128_encode_small(con, 0x1010);
	printf("0x1010 encoded to:%x %x\n", con[1], con[0]);
	uleb128_decode_small(con, &val);
	printf("result decoded to: %x\n", val);
}

void show_xbzrle(uint8_t *buf, int len)
{
	bool is_zrun = true;
	int i;

	for (i = 0; i < len; is_zrun = !is_zrun) {
		uint32_t val, j;

		i += uleb128_decode_small(buf + i, &val);

		printf("%d %szeros ", val, is_zrun?"":"non-");

		if (!is_zrun) {
			for (j = 0; j < val; j++)
				printf("%x ", *(buf + i + j));
			i += val;
		}

		printf("\n");

	}
}

void xbzrle_test()
{
	uint8_t old_buf[8] =
		{0x12, 0x27, 0x33, 0x45, 0x52, 0x60, 0x71, 0x86};
	uint8_t new_buf[8] =
		{0x13, 0x28, 0x33, 0x45, 0x52, 0x60, 0x71, 0x88};
	uint8_t dst[8];
	int len;

	len = xbzrle_encode_buffer(old_buf, new_buf, 8, dst, 8);
	show_xbzrle(dst, len);
}

int main()
{
	//leb128_encode_decode();

	xbzrle_test();

	return 0;
}
