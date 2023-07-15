/*
   Copyright (C)  2000    Daniel A. Atkinson  <DanAtk@aol.com>
   Copyright (C)  2004    Ivano Primi  <ivprimi@libero.it>    

   This file is part of the HPA Library.

   The HPA Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The HPA Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the HPA Library; if not, write to the Free
   Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
   02110-1301 USA.
*/

/* Constants file for XDIM=7
*/

const int xItt_div = 2;
const int xK_tanh = 5;
const int xMS_exp = 21;
const int xMS_hyp = 25;
const int xMS_trg = 31;

const xpr xPi4 = { {0x3ffe, 0xc90f, 0xdaa2, 0x2168,
			   0xc234, 0xc4c6, 0x628b, 0x80dc}
};
const xpr xPi2 = { {0x3fff, 0xc90f, 0xdaa2, 0x2168,
			   0xc234, 0xc4c6, 0x628b, 0x80dc}
};
const xpr xPi = { {0x4000, 0xc90f, 0xdaa2, 0x2168,
			  0xc234, 0xc4c6, 0x628b, 0x80dc}
};
const xpr xEe = { {0x4000, 0xadf8, 0x5458, 0xa2bb,
			  0x4a9a, 0xafdc, 0x5620, 0x273d}
};
const xpr xLn2 = { {0x3ffe, 0xb172, 0x17f7, 0xd1cf,
			   0x79ab, 0xc9e3, 0xb398, 0x03f3}
};
const xpr xLn10 = { {0x4000, 0x935d, 0x8ddd, 0xaaa8,
			    0xac16, 0xea56, 0xd62b, 0x82d3}
};
const xpr xSqrt2 = { {0x3fff, 0xb504, 0xf333, 0xf9de,
			     0x6484, 0x597d, 0x89b3, 0x754b}
};

const xpr xLog2_e = { {0x3fff, 0xb8aa, 0x3b29, 0x5c17,
			      0xf0bb, 0xbe87, 0xfed0, 0x691d}
};
const xpr xLog2_10 = { {0x4000, 0xd49a, 0x784b, 0xcd1b,
			       0x8afe, 0x492b, 0xf6ff, 0x4db0}
};
const xpr xLog10_e = { {0x3ffd, 0xde5b, 0xd8a9, 0x3728,
			       0x7195, 0x355b, 0xaaaf, 0xad34}
};

const xpr xRndcorr = { {0x3ffe, 0x8000, 0x0000, 0x0000,
			       0x0000, 0x0000, 0x0000, 0x0068}
};

const xpr xFixcorr = { {0x3f97, 0xc000, 0x0000, 0x0000,
			       0x0000, 0x0000, 0x0000, 0x0000}
};

const xpr xNaN = { {0x0000, 0xffff, 0xffff, 0xffff,
			   0xffff, 0xffff, 0xffff, 0xffff}
};
const xpr HPA_MIN = { {0x0001, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFE, 0xC979 } };
const xpr HPA_MAX = { {0x7FFC, 0x8000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x9B43 } };
