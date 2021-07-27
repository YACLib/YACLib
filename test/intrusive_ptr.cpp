#include <yaclib/container/counter.hpp>
#include <yaclib/container/intrusive_ptr.hpp>

#include <gtest/gtest.h>

namespace {

using namespace yaclib;

using container::intrusive::Ptr;

class Core : public IRef {
 public:
  static size_t sInstances;

  Core(const Core&) = delete;
  Core& operator=(const Core&) = delete;

  Core() {
    ++sInstances;
  }

  ~Core() {
    --sInstances;
  }
};

size_t Core::sInstances = 0;

class X : public Core {};
class Y : public X {};

using CounterX = container::Counter<X>;
using CounterY = container::Counter<Y>;

GTEST_TEST(ctor, default) {
  Ptr<Core> px;
  EXPECT_EQ(px.Get(), nullptr);
  EXPECT_EQ(px, nullptr);
  EXPECT_EQ(nullptr, px);
}

GTEST_TEST(ctor, pointer) {
  EXPECT_EQ(Core::sInstances, 0);
  {
    Ptr<Core> px{nullptr};
    EXPECT_EQ(px, nullptr);
  }
  EXPECT_EQ(Core::sInstances, 0);
  {
    Ptr<Core> px{nullptr, false};
    EXPECT_EQ(px, nullptr);
  }
  EXPECT_EQ(Core::sInstances, 0);
  {
    auto* p = new CounterX;
    EXPECT_EQ(p->GetRef(), 0);

    EXPECT_EQ(Core::sInstances, 1);

    Ptr<Core> px{p};
    EXPECT_EQ(px.Get(), p);
    EXPECT_EQ(px, p);
    EXPECT_EQ(p, px);
    EXPECT_EQ(p->GetRef(), 1);
  }
  EXPECT_EQ(Core::sInstances, 0);
  {
    auto* p = new CounterX;
    EXPECT_EQ(p->GetRef(), 0);

    p->IncRef();
    p->IncRef();
    EXPECT_EQ(p->GetRef(), 2);

    Ptr<Core> pb{p, false};
    {
      Ptr<CounterX> pc{p, false};
      EXPECT_EQ(pb, pc);
      EXPECT_EQ(p->GetRef(), 2);
    }
    EXPECT_EQ(p->GetRef(), 1);
  }
  EXPECT_EQ(Core::sInstances, 0);
}

GTEST_TEST(ctor, copy) {
  EXPECT_EQ(Core::sInstances, 0);
  {
    Ptr<CounterX> pc1;
    Ptr<CounterX> pc2{pc1};
    EXPECT_EQ(pc1, pc2);
  }
  EXPECT_EQ(Core::sInstances, 0);
  {
    Ptr<Y> py;
    Ptr<X> px{py};
    EXPECT_EQ(py, px);
  }
  EXPECT_EQ(Core::sInstances, 0);
  {
    Ptr<X> px1(new CounterX);
    Ptr<X> px2(px1);
    EXPECT_EQ(px1, px2);
    EXPECT_EQ(Core::sInstances, 1);
  }
  EXPECT_EQ(Core::sInstances, 0);
  {
    Ptr<Y> py{new CounterY};
    Ptr<X> px(py);
    EXPECT_EQ(py, px);
    EXPECT_EQ(Core::sInstances, 1);
  }
  EXPECT_EQ(Core::sInstances, 0);
}

GTEST_TEST(dtor, simple) {
  EXPECT_EQ(Core::sInstances, 0);
  {
    Ptr<CounterX> pc1;
    Ptr<CounterX> pc2{pc1};
    EXPECT_EQ(pc1, pc2);
  }
  EXPECT_EQ(Core::sInstances, 0);

  {
    auto x = new CounterX;
    Ptr<X> px1(x);
    EXPECT_EQ(x->GetRef(), 1);
    EXPECT_EQ(Core::sInstances, 1);
    {
      Ptr<X> px2(x);
      EXPECT_EQ(x->GetRef(), 2);
      EXPECT_EQ(Core::sInstances, 1);
    }
    EXPECT_EQ(x->GetRef(), 1);
    EXPECT_EQ(Core::sInstances, 1);
  }
  EXPECT_EQ(Core::sInstances, 0);
}

GTEST_TEST(assign, copy) {
  EXPECT_EQ(Core::sInstances, 0);

  {
    Ptr<X> p1;

    p1 = p1;

    EXPECT_EQ(p1, p1);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    Ptr<X> p2;

    p1 = p2;

    EXPECT_EQ(p1, p2);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    Ptr<X> p3(p1);

    p1 = p3;

    EXPECT_EQ(p1, p3);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    EXPECT_EQ(Core::sInstances, 0);

    Ptr<X> p4(new CounterX);

    EXPECT_EQ(Core::sInstances, 1);

    p1 = p4;

    EXPECT_EQ(Core::sInstances, 1);

    EXPECT_EQ(p1, p4);

    EXPECT_EQ(static_cast<CounterX&>(*p1).GetRef(), 2);

    p1 = p2;

    EXPECT_EQ(p1, p2);
    EXPECT_EQ(Core::sInstances, 1);
    EXPECT_EQ(static_cast<CounterX&>(*p4).GetRef(), 1);

    p4 = p3;

    EXPECT_EQ(p4, p3);
    EXPECT_EQ(Core::sInstances, 0);
  }
}

GTEST_TEST(assign, conversation) {
  EXPECT_EQ(Core::sInstances, 0);
  {
    Ptr<X> p1;

    Ptr<Y> p2;

    p1 = p2;
    EXPECT_EQ(p1, p2);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    EXPECT_EQ(Core::sInstances, 0);

    Ptr<Y> p4(new CounterY);

    EXPECT_EQ(Core::sInstances, 1);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 1);

    Ptr<X> p5(p4);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 2);

    p1 = p4;

    EXPECT_EQ(Core::sInstances, 1);

    EXPECT_EQ(p1, p4);

    EXPECT_EQ(static_cast<CounterY&>(*p1).GetRef(), 3);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 3);

    p1 = p2;

    EXPECT_EQ(p1, p2);
    EXPECT_EQ(Core::sInstances, 1);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 2);

    p4 = p2;
    p5 = p2;

    EXPECT_EQ(p4, p2);
    EXPECT_EQ(Core::sInstances, 0);
  }
}

GTEST_TEST(assign, pointer) {
  EXPECT_EQ(Core::sInstances, 0);
  {
    Ptr<X> p1;

    p1 = p1.Get();

    EXPECT_EQ(p1, p1);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    Ptr<X> p2;

    p1 = p2.Get();

    EXPECT_EQ(p1, p2);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    Ptr<X> p3(p1);

    p1 = p3.Get();

    EXPECT_EQ(p1, p3);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    EXPECT_EQ(Core::sInstances, 0);

    Ptr<X> p4(new CounterX);

    EXPECT_EQ(Core::sInstances, 1);

    p1 = p4.Get();

    EXPECT_EQ(Core::sInstances, 1);

    EXPECT_EQ(p1, p4);

    EXPECT_EQ(static_cast<CounterX&>(*p1).GetRef(), 2);

    p1 = p2.Get();

    EXPECT_EQ(p1, p2);
    EXPECT_EQ(Core::sInstances, 1);

    p4 = p3.Get();

    EXPECT_EQ(p4, p3);
    EXPECT_EQ(Core::sInstances, 0);
  }

  {
    Ptr<X> p1;
    Ptr<Y> p2;

    p1 = p2.Get();

    EXPECT_EQ(p1, p2);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    EXPECT_EQ(Core::sInstances, 0);

    Ptr<Y> p4(new CounterY);

    EXPECT_EQ(Core::sInstances, 1);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 1);

    Ptr<X> p5(p4);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 2);

    p1 = p4.Get();

    EXPECT_EQ(Core::sInstances, 1);

    EXPECT_EQ(p1, p4);

    EXPECT_EQ(static_cast<CounterY&>(*p1).GetRef(), 3);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 3);

    p1 = p2.Get();

    EXPECT_EQ(p1, p2);
    EXPECT_EQ(Core::sInstances, 1);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 2);

    p4 = p2.Get();
    p5 = p2.Get();

    EXPECT_EQ(p4, p2);
    EXPECT_EQ(Core::sInstances, 0);
  }
}
//
// GTEST_TEST(reset, simple) {
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    boost::intrusive_ptr<X> px;
//    BOOST_TEST(px.get() == 0);
//
//    px.reset();
//    BOOST_TEST(px.get() == 0);
//
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//    BOOST_TEST(N::base::instances == 1);
//
//    px.reset(p);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//
//    px.reset();
//    BOOST_TEST(px.get() == 0);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    boost::intrusive_ptr<X> px(new X);
//    BOOST_TEST(N::base::instances == 1);
//
//    px.reset(0);
//    BOOST_TEST(px.get() == 0);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    boost::intrusive_ptr<X> px(new X);
//    BOOST_TEST(N::base::instances == 1);
//
//    px.reset(0, false);
//    BOOST_TEST(px.get() == 0);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    boost::intrusive_ptr<X> px(new X);
//    BOOST_TEST(N::base::instances == 1);
//
//    px.reset(0, true);
//    BOOST_TEST(px.get() == 0);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//
//    BOOST_TEST(N::base::instances == 1);
//
//    boost::intrusive_ptr<X> px;
//    BOOST_TEST(px.get() == 0);
//
//    px.reset(p, true);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//
//    BOOST_TEST(N::base::instances == 1);
//
//    intrusive_ptr_add_ref(p);
//    BOOST_TEST(p->use_count() == 1);
//
//    boost::intrusive_ptr<X> px;
//    BOOST_TEST(px.get() == 0);
//
//    px.reset(p, false);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    boost::intrusive_ptr<X> px(new X);
//    BOOST_TEST(px.get() != 0);
//    BOOST_TEST(px->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 1);
//
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//
//    BOOST_TEST(N::base::instances == 2);
//
//    px.reset(p);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 1);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    boost::intrusive_ptr<X> px(new X);
//    BOOST_TEST(px.get() != 0);
//    BOOST_TEST(px->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 1);
//
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//
//    BOOST_TEST(N::base::instances == 2);
//
//    px.reset(p, true);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 1);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    boost::intrusive_ptr<X> px(new X);
//    BOOST_TEST(px.get() != 0);
//    BOOST_TEST(px->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 1);
//
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//
//    intrusive_ptr_add_ref(p);
//    BOOST_TEST(p->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 2);
//
//    px.reset(p, false);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 1);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//}
//
// GTEST_TEST(access, simple) {
//  {
//    boost::intrusive_ptr<X> px;
//    BOOST_TEST(px ? false : true);
//    BOOST_TEST(!px);
//
//    BOOST_TEST(get_pointer(px) == px.get());
//  }
//
//  {
//    boost::intrusive_ptr<X> px(0);
//    BOOST_TEST(px ? false : true);
//    BOOST_TEST(!px);
//
//    BOOST_TEST(get_pointer(px) == px.get());
//  }
//
//  {
//    boost::intrusive_ptr<X> px(new X);
//    BOOST_TEST(px ? true : false);
//    BOOST_TEST(!!px);
//    BOOST_TEST(&*px == px.get());
//    BOOST_TEST(px.operator->() == px.get());
//
//    BOOST_TEST(get_pointer(px) == px.get());
//  }
//
//  {
//    boost::intrusive_ptr<X> px;
//    X* detached = px.detach();
//    BOOST_TEST(px.get() == 0);
//    BOOST_TEST(detached == 0);
//  }
//
//  {
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//
//    boost::intrusive_ptr<X> px(p);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//
//    X* detached = px.detach();
//    BOOST_TEST(px.get() == 0);
//
//    BOOST_TEST(detached == p);
//    BOOST_TEST(detached->use_count() == 1);
//
//    delete detached;
//  }
//}

// GTEST_TEST(swap, simple) {
//   {
//     boost::intrusive_ptr<X> px;
//     boost::intrusive_ptr<X> px2;
//
//     px.swap(px2);
//
//     BOOST_TEST(px.get() == 0);
//     BOOST_TEST(px2.get() == 0);
//
//     using std::swap;
//     swap(px, px2);
//
//     BOOST_TEST(px.get() == 0);
//     BOOST_TEST(px2.get() == 0);
//   }
//
//   {
//     X* p = new X;
//     boost::intrusive_ptr<X> px;
//     boost::intrusive_ptr<X> px2(p);
//     boost::intrusive_ptr<X> px3(px2);
//
//     px.swap(px2);
//
//     BOOST_TEST(px.get() == p);
//     BOOST_TEST(px->use_count() == 2);
//     BOOST_TEST(px2.get() == 0);
//     BOOST_TEST(px3.get() == p);
//     BOOST_TEST(px3->use_count() == 2);
//
//     using std::swap;
//     swap(px, px2);
//
//     BOOST_TEST(px.get() == 0);
//     BOOST_TEST(px2.get() == p);
//     BOOST_TEST(px2->use_count() == 2);
//     BOOST_TEST(px3.get() == p);
//     BOOST_TEST(px3->use_count() == 2);
//   }
//
//   {
//     X* p1 = new X;
//     X* p2 = new X;
//     boost::intrusive_ptr<X> px(p1);
//     boost::intrusive_ptr<X> px2(p2);
//     boost::intrusive_ptr<X> px3(px2);
//
//     px.swap(px2);
//
//     BOOST_TEST(px.get() == p2);
//     BOOST_TEST(px->use_count() == 2);
//     BOOST_TEST(px2.get() == p1);
//     BOOST_TEST(px2->use_count() == 1);
//     BOOST_TEST(px3.get() == p2);
//     BOOST_TEST(px3->use_count() == 2);
//
//     using std::swap;
//     swap(px, px2);
//
//     BOOST_TEST(px.get() == p1);
//     BOOST_TEST(px->use_count() == 1);
//     BOOST_TEST(px2.get() == p2);
//     BOOST_TEST(px2->use_count() == 2);
//     BOOST_TEST(px3.get() == p2);
//     BOOST_TEST(px3->use_count() == 2);
//   }
// }
//
// namespace n_comparison {
//
// template <class T, class U>
// void test2(boost::intrusive_ptr<T> const& p, boost::intrusive_ptr<U> const&
// q) {
//   BOOST_TEST((p == q) == (p.get() == q.get()));
//   BOOST_TEST((p != q) == (p.get() != q.get()));
// }
//
// template <class T>
// void test3(boost::intrusive_ptr<T> const& p, boost::intrusive_ptr<T> const&
// q) {
//   BOOST_TEST((p == q) == (p.get() == q.get()));
//   BOOST_TEST((p.get() == q) == (p.get() == q.get()));
//   BOOST_TEST((p == q.get()) == (p.get() == q.get()));
//   BOOST_TEST((p != q) == (p.get() != q.get()));
//   BOOST_TEST((p.get() != q) == (p.get() != q.get()));
//   BOOST_TEST((p != q.get()) == (p.get() != q.get()));
//
//   // 'less' moved here as a g++ 2.9x parse error workaround
//   std::less<T*> less;
//   BOOST_TEST((p < q) == less(p.get(), q.get()));
// }
//
// void test() {
//   {
//     boost::intrusive_ptr<X> px;
//     test3(px, px);
//
//     boost::intrusive_ptr<X> px2;
//     test3(px, px2);
//
//     boost::intrusive_ptr<X> px3(px);
//     test3(px3, px3);
//     test3(px, px3);
//   }
//
//   {
//     boost::intrusive_ptr<X> px;
//
//     boost::intrusive_ptr<X> px2(new X);
//     test3(px, px2);
//     test3(px2, px2);
//
//     boost::intrusive_ptr<X> px3(new X);
//     test3(px2, px3);
//
//     boost::intrusive_ptr<X> px4(px2);
//     test3(px2, px4);
//     test3(px4, px4);
//   }
//
//   {
//     boost::intrusive_ptr<X> px(new X);
//
//     boost::intrusive_ptr<Y> py(new Y);
//     test2(px, py);
//
//     boost::intrusive_ptr<X> px2(py);
//     test2(px2, py);
//     test3(px, px2);
//     test3(px2, px2);
//   }
// }
//
// }  // namespace n_comparison
//
// namespace n_static_cast {
//
// void test() {
//   {
//     boost::intrusive_ptr<X> px(new Y);
//
//     boost::intrusive_ptr<Y> py = boost::static_pointer_cast<Y>(px);
//     BOOST_TEST(px.get() == py.get());
//     BOOST_TEST(px->use_count() == 2);
//     BOOST_TEST(py->use_count() == 2);
//
//     boost::intrusive_ptr<X> px2(py);
//     BOOST_TEST(px2.get() == px.get());
//   }
//
//   BOOST_TEST(N::base::instances == 0);
//
//   {
//     boost::intrusive_ptr<Y> py =
//         boost::static_pointer_cast<Y>(boost::intrusive_ptr<X>(new Y));
//     BOOST_TEST(py.get() != 0);
//     BOOST_TEST(py->use_count() == 1);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
// }
//
// }  // namespace n_static_cast
//
// namespace n_const_cast {
//
// void test() {
//   {
//     boost::intrusive_ptr<X const> px;
//
//     boost::intrusive_ptr<X> px2 = boost::const_pointer_cast<X>(px);
//     BOOST_TEST(px2.get() == 0);
//   }
//
//   {
//     boost::intrusive_ptr<X> px2 =
//         boost::const_pointer_cast<X>(boost::intrusive_ptr<X const>());
//     BOOST_TEST(px2.get() == 0);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
//
//   {
//     boost::intrusive_ptr<X const> px(new X);
//
//     boost::intrusive_ptr<X> px2 = boost::const_pointer_cast<X>(px);
//     BOOST_TEST(px2.get() == px.get());
//     BOOST_TEST(px2->use_count() == 2);
//     BOOST_TEST(px->use_count() == 2);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
//
//   {
//     boost::intrusive_ptr<X> px =
//         boost::const_pointer_cast<X>(boost::intrusive_ptr<X const>(new X));
//     BOOST_TEST(px.get() != 0);
//     BOOST_TEST(px->use_count() == 1);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
// }
//
// }  // namespace n_const_cast
//
// namespace n_dynamic_cast {
//
// void test() {
//   {
//     boost::intrusive_ptr<X> px;
//
//     boost::intrusive_ptr<Y> py = boost::dynamic_pointer_cast<Y>(px);
//     BOOST_TEST(py.get() == 0);
//   }
//
//   {
//     boost::intrusive_ptr<Y> py =
//         boost::dynamic_pointer_cast<Y>(boost::intrusive_ptr<X>());
//     BOOST_TEST(py.get() == 0);
//   }
//
//   {
//     boost::intrusive_ptr<X> px(static_cast<X*>(0));
//
//     boost::intrusive_ptr<Y> py = boost::dynamic_pointer_cast<Y>(px);
//     BOOST_TEST(py.get() == 0);
//   }
//
//   {
//     boost::intrusive_ptr<Y> py = boost::dynamic_pointer_cast<Y>(
//         boost::intrusive_ptr<X>(static_cast<X*>(0)));
//     BOOST_TEST(py.get() == 0);
//   }
//
//   {
//     boost::intrusive_ptr<X> px(new X);
//
//     boost::intrusive_ptr<Y> py = boost::dynamic_pointer_cast<Y>(px);
//     BOOST_TEST(py.get() == 0);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
//
//   {
//     boost::intrusive_ptr<Y> py =
//         boost::dynamic_pointer_cast<Y>(boost::intrusive_ptr<X>(new X));
//     BOOST_TEST(py.get() == 0);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
//
//   {
//     boost::intrusive_ptr<X> px(new Y);
//
//     boost::intrusive_ptr<Y> py = boost::dynamic_pointer_cast<Y>(px);
//     BOOST_TEST(py.get() == px.get());
//     BOOST_TEST(py->use_count() == 2);
//     BOOST_TEST(px->use_count() == 2);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
//
//   {
//     boost::intrusive_ptr<X> px(new Y);
//
//     boost::intrusive_ptr<Y> py =
//         boost::dynamic_pointer_cast<Y>(boost::intrusive_ptr<X>(new Y));
//     BOOST_TEST(py.get() != 0);
//     BOOST_TEST(py->use_count() == 1);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
// }
//
// }  // namespace n_dynamic_cast
//
// namespace n_transitive {
//
// struct X : public N::base {
//   boost::intrusive_ptr<X> next;
// };
//
// void test() {
//   boost::intrusive_ptr<X> p(new X);
//   p->next = boost::intrusive_ptr<X>(new X);
//   BOOST_TEST(!p->next->next);
//   p = p->next;
//   BOOST_TEST(!p->next);
// }
//
// }  // namespace n_transitive
//
// namespace n_report_1 {
//
// class foo : public N::base {
//  public:
//   foo() : m_self(this) {
//   }
//
//   void suicide() {
//     m_self = 0;
//   }
//
//  private:
//   boost::intrusive_ptr<foo> m_self;
// };
//
// void test() {
//   foo* foo_ptr = new foo;
//   foo_ptr->suicide();
// }
//
// }  // namespace n_report_1

}  // namespace
