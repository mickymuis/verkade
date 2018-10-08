#include <vector>
#include <algorithm>

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "matrix.h"

extern "C" {
#include "mmio.h"
}

/*
 * Load matrix market file
 */

struct Element
{
  size_t row, col;
  double val;

  Element(int row, int col, double val) : row(row), col(col), val(val) { }
};

inline bool operator==(const struct Element &a, const struct Element &b)
{
  return a.row == b.row && a.col == b.col && a.val == b.val;
}

inline bool operator!=(const struct Element &a, const struct Element &b)
{
  return !operator==(a, b);
}

inline bool operator<(const struct Element &a, const struct Element &b)
{
  if (a.row < b.row)
    return true;

  if (a.row == b.row && a.col < b.col)
    return true;

  return false;
}



static bool
read_matrix_market(const char *filename,
                   std::vector<Element> &elements,
                   size_t &n_rows, size_t &n_cols)
{
  FILE *fh = fopen(filename, "r");
  if (!fh)
    {
      perror("fopen");
      return false;
    }

  printf("Reading matrix '%s'...\n", filename);

  MM_typecode matcode;
  if (mm_read_banner(fh, &matcode) != 0)
    {
      fprintf(stderr, "(e) mm_read_banner failed\n");
      return false;
    }

  int M, N, nz;
  int ret_code;
  ret_code = mm_read_mtx_crd_size(fh, &M, &N, &nz);
  if (ret_code != 0)
    {
      fprintf(stderr, "(e) mm_read_mtx_crd_size failed\n");
      return false;
    }

  n_rows = M;
  n_cols = N;

  for (int i = 0; i < nz; i++)
    {
      int row, col;
      double val;

      if (mm_is_pattern(matcode))
        {
          fscanf(fh, "%d %d\n", &row, &col);
          val = 1.0;
        }
      else
        fscanf(fh, "%d %d %lg\n", &row, &col, &val);

      row--; /* adjust from 1-based to 0-based */
      col--;

      elements.push_back(Element(row, col, val));
      if (mm_is_symmetric(matcode) && row != col)
        elements.push_back(Element(col, row, val));
    }

  std::sort(elements.begin(), elements.end());
  fclose(fh);

  return true;
}

/*
 * Transfer matrix elements into Compressed Row Storage structure
 */

static void
load_elements(const std::vector<Element> &elements,
              double values[],
              indx_t col_ind[],
              indx_t row_ptr_begin[], indx_t row_ptr_end[])
{
  indx_t current_val = 0;
  indx_t current_row = 0;
  row_ptr_begin[0] = 0;

  for (std::vector<Element>::const_iterator it = elements.begin();
       it != elements.end(); ++it)
    {
      if (it->row != current_row)
        {
          if (current_row + 1 != it->row)
            {
              fprintf(stderr, "(e) Row skipping not implemented.\n");
              abort();
            }

          row_ptr_end[current_row] = current_val - 1;
          current_row++;
          row_ptr_begin[current_row] = current_val;
        }

      values[current_val] = it->val;
      col_ind[current_val] = it->col;
      current_val++;
    }

  row_ptr_end[current_row] = current_val - 1;
}

void
dump_nonzeros(const size_t n_rows,
              const double values[],
              const indx_t col_ind[],
              const indx_t row_ptr_begin[],
              const indx_t row_ptr_end[])
{
  for (size_t row = 0; row < n_rows; ++row)
    {
      for (indx_t idx = row_ptr_begin[row]; idx <= row_ptr_end[row]; ++idx)
        {
          printf("%ld %ld %f\n", row, col_ind[idx], values[idx]);
        }
    }
}

bool
load_matrix_market(const char *filename,
                   const size_t max_n_elements,
                   const size_t max_n_rows,
                   size_t &nnz, size_t &n_rows, size_t &n_cols,
                   double values[],
                   indx_t col_ind[],
                   indx_t row_ptr_begin[], indx_t row_ptr_end[])
{
  std::vector<Element> elements;

  if (!read_matrix_market(filename, elements, n_rows, n_cols))
    return false;

  if (elements.size() >= (size_t)max_n_elements || n_rows >= max_n_rows)
    return false;

  nnz = elements.size();
  load_elements(elements, values, col_ind, row_ptr_begin, row_ptr_end);

  printf("(i) Import ok: %ld x %ld matrix, %ld non-zeroes\n",
         n_rows, n_cols, nnz);

  return true;
}
