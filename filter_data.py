import pandas as pd
import io
import numpy as np

file_path = 'Lengkap_ACD.csv'

try:
    df = pd.read_csv(file_path)
    print(f"\nBerhasil memuat data dari '{file_path}'. Jumlah baris awal: {len(df)}")
except FileNotFoundError:
    print(f"Error: File '{file_path}' tidak ditemukan.")
    exit()
except Exception as e:
    print(f"Error saat memuat data: {e}")
    exit()

target_correlation = 0.5
input_cols = ['IR','AGE', 'HR'] #'Infrared'
output_col = 'ACD'
min_rows_threshold = 10

def calculate_correlations(dataframe):
    if len(dataframe) < 2:
        return pd.Series(dtype=float)
    if dataframe[input_cols + [output_col]].std().eq(0).any():
         return pd.Series(index=input_cols, data=np.nan)
    try:
        return dataframe.corr(method='pearson')[output_col][input_cols]
    except Exception as e:
        print(f"Error calculating correlation: {e}")
        return pd.Series(dtype=float)

current_df = df.copy()
iteration = 0

print("\nMemulai proses penghapusan iteratif...")

while True:
    iteration += 1
    print(f"\n--- Iterasi {iteration} ---")
    print(f"Jumlah baris saat ini: {len(current_df)}")

    if len(current_df) <= min_rows_threshold:
        print(f"Berhenti: Jumlah baris ({len(current_df)}) di bawah atau sama dengan threshold ({min_rows_threshold}).")
        break

    correlations = calculate_correlations(current_df)
    if correlations.empty or correlations.isnull().all():
         print("Berhenti: Tidak dapat menghitung korelasi (mungkin karena data tidak cukup atau varians nol).")
         break

    print("Korelasi saat ini:")
    print(correlations)
    min_corr_value = correlations.min()
    min_corr_var = correlations.idxmin() if not correlations.isnull().all() else "N/A"
    print(f"Korelasi minimum saat ini ({min_corr_var}): {min_corr_value:.4f}")

    if not correlations.isnull().any() and (correlations > target_correlation).all():
        print(f"\nTarget korelasi > {target_correlation} tercapai untuk semua variabel!")
        break

    best_min_corr_after_removal = -np.inf
    best_row_index_to_remove = None

    indices_to_try = current_df.index

    for idx in indices_to_try:
        temp_df = current_df.drop(idx)
        if len(temp_df) < 2:
            continue

        temp_correlations = calculate_correlations(temp_df)

        if not temp_correlations.empty and not temp_correlations.isnull().any():
            current_min_corr_after_removal = temp_correlations.min()

            if current_min_corr_after_removal > best_min_corr_after_removal:
                best_min_corr_after_removal = current_min_corr_after_removal
                best_row_index_to_remove = idx

    if best_row_index_to_remove is None or best_min_corr_after_removal <= min_corr_value:
        print("\nBerhenti: Tidak ada baris yang dapat dihapus untuk meningkatkan korelasi minimum secara signifikan.")
        break

    print(f"Menghapus baris dengan index {best_row_index_to_remove}...")
    print(f"(Menghapus baris ini meningkatkan korelasi minimum menjadi ~{best_min_corr_after_removal:.4f})")
    current_df = current_df.drop(best_row_index_to_remove)

print("\n--- Proses Selesai ---")
print(f"Jumlah baris akhir: {len(current_df)}")

final_correlations = calculate_correlations(current_df)
if not final_correlations.empty:
    print("Korelasi Akhir:")
    print(final_correlations)
else:
    print("Tidak dapat menghitung korelasi akhir (data tidak cukup).")

output_file = 'ACD_.csv'
try:
    current_df = current_df[df.columns]
    current_df.to_csv(output_file, header=False, index=False)
    print(f"\nData yang telah difilter disimpan ke '{output_file}' tanpa header.")
except Exception as e:
    print(f"\nError saat menyimpan file: {e}")

try:
    df_check = pd.read_csv(output_file, header=None, names=df.columns)
    print(f"\nVerifikasi: Berhasil membaca kembali '{output_file}'. Jumlah baris: {len(df_check)}")
except Exception as e:
    print(f"\nError saat verifikasi file tersimpan: {e}")
