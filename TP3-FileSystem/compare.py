def comparar_archivos(file1, file2):
"""
    Función auxiliar para comparar las salidas propias con las de la cátedra.
    Compara dos archivos línea por línea e imprime las diferencias encontradas.
"""

    with open(file1, 'r') as f1, open(file2, 'r') as f2:
        lineas1 = f1.readlines()
        lineas2 = f2.readlines()

    max_lineas = max(len(lineas1), len(lineas2))
    diferencia_encontrada = False

    for i in range(max_lineas):
        l1 = lineas1[i].strip() if i < len(lineas1) else "<NO HAY LINEA>"
        l2 = lineas2[i].strip() if i < len(lineas2) else "<NO HAY LINEA>"

        if l1 != l2:
            diferencia_encontrada = True
            print(f"Diferencia en la línea {i+1}:")
            print(f"  {file1}: {l1}")
            print(f"  {file2}: {l2}")
            print("-" * 50)

    if not diferencia_encontrada:
        print("✅ Los archivos son idénticos.")

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 3:
        print("Uso: python3 comparar_archivos.py archivo1 archivo2")
    else:
        comparar_archivos(sys.argv[1], sys.argv[2])
