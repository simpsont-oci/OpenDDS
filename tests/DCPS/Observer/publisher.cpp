#include "Domain.h"
#include "TestObserver.h"

#include <tests/DCPS/ConsolidatedMessengerIdl/MessengerTypeSupportImpl.h>
#include <tests/Utils/StatusMatching.h>

#include <dds/DCPS/EntityImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <assert.h>
#include <iostream>

class Publisher
{
public:
  Publisher(int argc, ACE_TCHAR* argv[]);
  int run();
private:
  int waitForSubscriber();
  Domain domain_;
  Messenger::MessageDataWriter_var writer_;
};

Publisher::Publisher(int argc, ACE_TCHAR* argv[]) : domain_(argc, argv, "Publisher")
{
  DDS::Publisher_var pub = domain_.participant->create_publisher(PUBLISHER_QOS_DEFAULT,
    DDS::PublisherListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(pub.in())) {
    throw ACE_TEXT("create_publisher failed.");
  }

  DDS::DataWriter_var dw = pub->create_datawriter(domain_.topic.in(), DATAWRITER_QOS_DEFAULT,
    DDS::DataWriterListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(dw.in())) {
    throw ACE_TEXT("create_datawriter failed.");
  }
  writer_ = Messenger::MessageDataWriter::_narrow(dw);
  if (CORBA::is_nil(writer_.in())) {
    throw ACE_TEXT("TestMsgDataWriter::_narrow failed.");
  }

  //auto entity = dynamic_cast<OpenDDS::DCPS::EntityImpl*>(pub.ptr());
  auto entity = dynamic_cast<OpenDDS::DCPS::EntityImpl*>(dw.ptr());
  entity->set_observer(OpenDDS::DCPS::make_rch<TestObserver>(), OpenDDS::DCPS::Observer::e_SAMPLE_SENT);
}

int Publisher::run()
{
  if (waitForSubscriber() != 0) return 1;

  //Message{"from", "subject", @key subject_id, "text", count, ull, source_pid}
  Messenger::Message msg = {"", "Observer", 1, "test", 1, 0, 0};
  DDS::InstanceHandle_t handle = writer_->register_instance(msg);
  for (msg.count = 1; msg.count <= Domain::N_MSG; ++msg.count) {
    DDS::ReturnCode_t r = writer_->write(msg, handle);
    if (r != ::DDS::RETCODE_OK) {
      std::cerr << "Publisher write returned code " << r << std::endl;
    }
    ACE_OS::sleep(ACE_Time_Value(0, 300000)); // sleep 300 ms
  }
  return 0;
}

int Publisher::waitForSubscriber()
{
  std::cout << "Publisher waiting for subscriber..." << std::endl;
  DDS::DataWriter_var writer = DDS::DataWriter::_narrow(writer_);
  return Utils::wait_match(writer, Domain::N_READER);
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    Publisher pub(argc, argv);
    return pub.run();
  } catch (...) {
    return 1;
  }
}
