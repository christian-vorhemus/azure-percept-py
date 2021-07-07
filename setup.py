from distutils.core import setup, Extension


def main():
    setup(name="azure-percept",
          version="1.0.0",
          description="Unofficially Python package to control the Azure Percept SoM",
          author="Christian Vorhemus",
          author_email="chris.vorhemus@outlook.com",
          ext_modules=[Extension("_hardware", ["azure/iot/percept/ext/perceptmodule.c"], extra_link_args=["-lasound", "-lpthread"])])


if __name__ == "__main__":
    main()
