#include "parser/databento.hpp"

#include <iostream>
#include <string>

#include "databento/file_stream.hpp"
#include "databento/log.hpp" // For NullLogReceiver

#include "str_utils.hpp"

/*
 *
auto header = record.Header()
header.length
header.rtype
header.publisher_id
header.instrument_id
header.ts_event
*/

using namespace ::databento;

std::ostream &operator<<(std::ostream &os, const MboMsg *p_mbo) {
  const auto &mbo = *p_mbo;
  os << fmt::KeyValue{"Price", mbo.price} << fmt::KeyValue{"Volume", mbo.size}
     << fmt::KeyValue{"Action", mbo.action} << fmt::KeyValue{"Side", mbo.side}
     << fmt::KeyValue{"Channel ID", mbo.channel_id}
     << fmt::KeyValue{"Flags", mbo.flags}
     //<< fmt::KeyValue{"TS delta", mbo.ts_in_delta}
     << fmt::KeyValue{"Sequence", mbo.sequence};
  return os;
}

NullLogReceiver log_receiver;

std::unique_ptr<DbnDecoder>
parser::databento::open(const std::filesystem::path &file_path) {
  InFileStream file_stream{file_path};
  auto decoder =
      std::make_unique<DbnDecoder>(&log_receiver, std::move(file_stream));

  const Metadata &metadata = decoder->DecodeMetadata();
  std::cout << "Metadata: " << metadata << std::endl;

  return decoder;
}

auto error(std::string_view context, std::string_view details) {
  std::ostringstream ss;
  ss << context << ": " << details;
  return Expected<const MboMsg *, std::string>(ss.str());
}

Expected<const MboMsg *, std::string>
parser::databento::parse_message(DbnDecoder &decoder) {
  if (const Record *record = decoder.DecodeRecord()) {
    if (record->RType() == RType::Mbo)
      return &record->Get<MboMsg>();
    return error("Databento parser", "Not a MBO record type");
  }

  return nullptr;
}
