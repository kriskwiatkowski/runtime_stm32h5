from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

class SymbolMapper:
    def __init__(self, elf_path):
        self.symbols = []
        with open(elf_path, 'rb') as f:
            elffile = ELFFile(f)
            # Find the symbol table section
            section = elffile.get_section_by_name('.symtab')
            if not section or not isinstance(section, SymbolTableSection):
                raise Exception("No symbol table found. Ensure firmware is not stripped.")

            # Store only function symbols with their start and end addresses
            for symbol in section.iter_symbols():
                if symbol['st_info']['type'] == 'STT_FUNC':
                    start_addr = symbol['st_value']
                    # Clear the Thumb bit (LSB) if present for accurate matching
                    start_addr &= ~1 
                    end_addr = start_addr + symbol['st_size']
                    self.symbols.append({
                        'name': symbol.name,
                        'start': start_addr,
                        'end': end_addr
                    })

    def get_function(self, addr):
        # Clear Thumb bit from sampled address
        addr &= ~1 
        for sym in self.symbols:
            if sym['start'] <= addr < sym['end']:
                return sym['name']
        return f"Unknown (0x{addr:08X})"

# Usage example:
# mapper = SymbolMapper("build/firmware.elf")
# print(mapper.get_function(0x08001234))
