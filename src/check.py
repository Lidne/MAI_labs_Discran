def main():
    input_data = []
    with open("input.txt", encoding="utf-8") as inp:
        input_data = list(map(lambda x: x.strip(), inp.readlines()))

    output_data = []
    with open("output.txt", encoding="utf-8") as out:
        output_data = list(map(lambda x: x.strip(), out.readlines()))

    test_data = []
    with open("tests.txt", encoding="utf-8") as test:
        test_data = list(map(lambda x: x.strip(), test.readlines()))

    with open("compared.txt", encoding="utf-8", mode="w") as comp:
        for index, i, o, t in zip(range(1, 1000000), input_data, output_data, test_data):
            if o != t:
                print(f"index: {index}, i: '{i}', o: '{o}', t: '{t}'", file=comp)


if __name__ == "__main__":
    main()
